/*
 * devns_init.c
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

#define LOG_TAG "devns-init"
#include <cutils/log.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/input.h>

#define DEVNS_FIFO "/dev/devns.fifo"
#define DEVNS_EVDEV "/dev/input/event0"

#define DEVNS_DELAY 100  /* delay in ms */

#define ALOGE_ERRNO(s)  ALOGE(s ": %s", strerror(errno))
#define ALOGW_ERRNO(s)  ALOGW(s ": %s", strerror(errno))

static int switch_back(void)
{
	int which = 0;
	int ret, fd;

	fd = open(DEVNS_FIFO, O_WRONLY);
	if (fd < 0) {
		ALOGE_ERRNO("Failed to open fifo");
		return -1;
	}

	ret = write(fd, &which, sizeof(which));
	if (ret != (int) sizeof(which)) {
		ALOGE_ERRNO("Failed to write to fifo");
		ret = -1;
	}

	close(fd);

	return ret;
}

static int wait_switch(void)
{
	int which, ret;

	while (1) {
		ret = read(STDIN_FILENO, &which, sizeof(which));

		if (ret < 0) {
			ALOGE_ERRNO("Failed to read from pipe");
			return ret;
		} else if (ret == 0) {
			ALOGE_ERRNO("End of file from pipe");
			return -1;
		}

		if (which)
			break;
	}

	return 0;
}

int main(int argc __attribute__((unused)),
	 char *argv[] __attribute__((unused)))
{
	struct fb_var_screeninfo var;
	unsigned char *fbbuf;
	long fbsize;
	int color;
	int fb_fd, ev_fd;
	int ret = -1;

	ALOGI("Service devns_init started");

	/* open video memory */
	if ((fb_fd = open("/dev/graphics/fb0", O_RDWR)) < 0) {
		ALOGE_ERRNO("Failed to open framebuffer");
		exit(1);
	}
	
	/* get variable display parameters */
	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var)) {
		ALOGE_ERRNO("Failed vscreeninfo ioctl");
		exit(1);
	}

	/* size of frame buffer =
	 * (X-resolution * Y-resolution * bytes per pixel) */
	fbsize = var.xres * var.yres * (var.bits_per_pixel/8);

	/* map video memory */
	if ((fbbuf = mmap(0, fbsize, PROT_READ|PROT_WRITE,
			  MAP_SHARED, fb_fd, 0)) == MAP_FAILED) {
		ALOGE_ERRNO("Failed to map framebuffer");
		exit(1);
	}

	/* open input evdev */
	ev_fd = open(DEVNS_EVDEV, O_RDONLY | O_NONBLOCK);
	if (ev_fd < 0) {
		ALOGE_ERRNO("Failed to open " DEVNS_EVDEV);
		exit(1);
	}

	color = 0;

	while (1) {
		ret = wait_switch();
		if (ret < 0)
			break;

		ALOGI("Switched to secondary devns");

		while (1) {
			struct input_event ev;
			int i;

			/* paint the screen */
			for (i = 0; i < fbsize; i++)
				*(fbbuf + i) = color;

			if (++color >= 256)
				color = 0;

			usleep(DEVNS_DELAY * 1000);

			/* force redraw- neede for emulator on x86+KVM */
			memset(&var, 0, sizeof(var));
			ret = ioctl(fb_fd, FBIOPAN_DISPLAY, &var);
			if (ret < 0)
				ALOGE_ERRNO("fb ioctl FBIOPAN_DISPLAY");

			/* wait for some input to switch back */
			ret = read(ev_fd, &ev, sizeof(struct input_event));
			if (ret < 0 && errno != EAGAIN)
				ALOGE_ERRNO("Failed read from evdev");

			if(ret >= 0 && ev.type == 1 && ev.value == 1)
				break;
		}

		if (ret < 0)
			break;

		ALOGI("Switching back to primary devns");

		if (switch_back() < 0)
			break;
	}

	munmap(fbbuf, fbsize);
	close(fb_fd);
	close(ev_fd);

	return (ret == 0 ? ret : 1);
}

