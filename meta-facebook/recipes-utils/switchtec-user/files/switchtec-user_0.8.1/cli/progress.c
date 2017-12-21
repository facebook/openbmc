/*
 * Microsemi Switchtec(tm) PCIe Management Command Line Interface
 * Copyright (c) 2017, Microsemi Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "progress.h"

#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static struct timeval start_time;

static void timeval_subtract(struct timeval *result,
			     struct timeval *x,
			     struct timeval *y)
{
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;

		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}

	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;

		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;
}

static void print_bar(int cur, int total)
{
	int bar_width;
	struct winsize w;
	int i;
	float progress = cur * 100.0 / total;
	int pos;

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	bar_width = w.ws_col - 33;
	pos = bar_width * cur / total;

	printf(" %3.0f%% [", progress);
	for (i = 0; i < bar_width; i++) {
		if (i < pos)
			putchar('=');
		else if (i == pos)
			putchar('>');
		else
			putchar(' ');
	}
	printf("] ");
}

static void print_time(struct timeval *interval)
{
	div_t minsec, hrmin;

	minsec = div(interval->tv_sec, 60);
	hrmin = div(minsec.quot, 60);

	printf("%d:%02d:%02d", hrmin.quot, hrmin.rem, minsec.rem);
}

static int calc_eta(int cur, int total, struct timeval *eta, double *rate)
{
	struct timeval now;
	struct timeval elapsed;
	double elaps_sec;
	double per_item;

	if (cur == 0)
		return -1;

	gettimeofday(&now, NULL);
	timeval_subtract(&elapsed, &now, &start_time);
	elaps_sec = elapsed.tv_sec + elapsed.tv_usec * 1e-6;

	per_item = elaps_sec / cur;
	elaps_sec = (per_item) * (total - cur);
	if (rate)
		*rate = 1 / per_item;

	eta->tv_sec = elaps_sec;

	return 0;
}

void progress_start(void)
{
	gettimeofday(&start_time, NULL);
}

void progress_update(int cur, int total)
{
	struct timeval eta;
	double rate = 0.0;

	print_bar(cur, total);

	printf("ETA:  ");
	if (calc_eta(cur, total, &eta, &rate))
		printf("-:--:--");
	else
		print_time(&eta);

	printf("  %3.0fkB/s ", rate / 1024);

	printf("\r");
	fflush(stdout);
}

void progress_finish(void)
{
	struct timeval now;
	struct timeval elapsed;

	gettimeofday(&now, NULL);
	timeval_subtract(&elapsed, &now, &start_time);

	print_bar(100, 100);
	printf("Time: ");
	print_time(&elapsed);
	printf("\n");
}
