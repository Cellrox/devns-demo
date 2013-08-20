/*
 * devns_switch.c
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DEVNS_FIFO "/dev/devns.fifo"

int main(int argc, char *argv[])
{
	char which;
	int ret;
	int fd;

	if (argc != 2) {
		printf("usage: %s 0|1\n", argv[0]);
		exit(1);
	}

	which = atoi(argv[1]);

	fd = open(DEVNS_FIFO, O_WRONLY);
	if (fd < 0) {
		printf("Failed to open fifo: %s\n", strerror(errno));
		exit(1);
	}

	ret = write(fd, &which, sizeof(which));
	if (ret != (int) sizeof(which))
		printf("Failed to write to fifo: %s\n", strerror(errno));

	close(fd);

	return 0;
}

