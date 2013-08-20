/*
 * devns.c
 *
 * Copyright (c) 2011-2013 Cellrox Ltd.
 *
 * This program is free software licensed under the GNU General Public
 * License Version 2 (GPL 2). You can distribute it and/or modify it under
 * the terms of the GPL 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GPL 2 license for more details.
 * The full GPL 2 License is included in this distribution in the file called
 * COPYING
 *
 * Cellrox can be contacted at devns@cellrox.com
 */

#define LOG_TAG "devns"
#include <cutils/log.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* Missing flags from bionic's sched.h, but available in kernel */
#define CLONE_NEWUTS		0x04000000	/* New utsname group? */
#define CLONE_NEWIPC		0x08000000	/* New ipcs */
#define CLONE_NEWUSER		0x10000000	/* New user namespace */
#define CLONE_NEWPID		0x20000000	/* New pid namespace */
#define CLONE_NEWNET		0x40000000	/* New network namespace */

/*
 * clients requesting a switch should contact via FIFO:
 * 1 - switch to secondary namespace
 * 2 - switch back to primary namespace
 */
#define DEVNS_FIFO "/dev/devns.fifo"
#define DEVNS_INIT "/system/bin/devns_init"
static char *devns_argv[2] = { "devns_init", NULL };

#define ALOGE_ERRNO(s)  ALOGE(s ": %s", strerror(errno))
#define ALOGW_ERRNO(s)  ALOGW(s ": %s", strerror(errno))

static int pipefd[2];

int clone(int (*fn)(void *), void *child_stack, int flags, void *arg, ...);

static int do_child(void *data)
{
	char **argv = data;

	if (dup2(pipefd[0], STDIN_FILENO) < 0) {
		ALOGE_ERRNO("dup2 failed");
		return 1;
	}

	close(pipefd[1]);

	execve(DEVNS_INIT, argv, NULL);

	/* not reached */
	ALOGE_ERRNO("exec failed");

	return 1;
}

static int do_clone(char **argv, unsigned long flags)
{
	pid_t pid;
	size_t stacksize = 4 * sysconf(_SC_PAGESIZE);
	void *stack;
	char *buf;

	buf = malloc(stacksize);
	if (!buf) {
		ALOGE_ERRNO("stack alloc falied");
		return -1;
	}

	stack = buf + stacksize;

	pid = clone(do_child, stack, flags | SIGCHLD, argv);
	if (pid < 0) {
		ALOGE_ERRNO("clone failed");
		free(buf);
		return -1;
	}

	return pid;
}

static int do_switch(pid_t pid)
{
	char buf[16];
	int which, fd, ret;

	which = (pid != 1 ? 1 : 0);

	ret = write(pipefd[1], &which, sizeof(which));
	if (ret < 0) {
		ALOGE_ERRNO("Failed to write to pipe");
		return ret;
	}

	fd = open("/proc/dev_ns/active_ns_pid", O_WRONLY);
	if (fd < 0) {
		ALOGW_ERRNO("open active_ns_pid failed");
		return 0;
	}

	sprintf(buf, "%d", pid);

	ret = write(fd, buf, strlen(buf));
	if (ret < 0)
		ALOGW_ERRNO("write active_ns_pid failed");

	close(fd);

	return 0;
}

void handler(int sig __attribute((unused)))
{
	/* ignore */
}

int main(int argc __attribute__((unused)),
	 char *argv[] __attribute__((unused)))
{
	int fifofd;
	char which;
	pid_t pid;
	int ret;

	ALOGI("Service devns started");

	/* pipe to communicate with child namespace */
	ret = pipe(pipefd);
	if (ret < 0) {
		ALOGE_ERRNO("Failed to create pipe");
		exit(1);
	}

	/* named pipe for switch requests */
	unlink(DEVNS_FIFO);
	if (mkfifo(DEVNS_FIFO, 0666) < 0) {
		ALOGE_ERRNO("Failed to create fifo");
		exit(1);
	}
	if (chmod(DEVNS_FIFO, 0666) < 0) {
		ALOGE_ERRNO("Failed to chmod fifo");
		exit(1);
	}

	pid = do_clone(devns_argv, CLONE_NEWNS|CLONE_NEWPID);
	if (pid < 0)
		exit(1);

	close(pipefd[0]);

	signal(SIGPIPE, handler);

	while (1) {
		fifofd = open(DEVNS_FIFO, O_RDONLY);
		if (fifofd < 0) {
			ALOGE_ERRNO("Failed to open fifo");
			break;
		}

		ret = read(fifofd, &which, sizeof(which));
		close(fifofd);

		if (ret < 0) {
			ALOGW_ERRNO("Failed to read from fifo");
			continue;
		} else if (ret < (int) sizeof(which)) {
			ALOGW("Read too short from fifo");
			continue;
		}

		ret = do_switch(which ? pid : 1);
		if (ret < 0) {
			ALOGE_ERRNO("Failed to switch");
			break;
		}

		close(fifofd);
	}

	kill(pid, SIGKILL);
	waitpid(pid, NULL, 0);

	/* not reached */

	return 1;
}
	
