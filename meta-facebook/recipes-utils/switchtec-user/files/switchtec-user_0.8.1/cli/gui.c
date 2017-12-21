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

#include "gui.h"
#include "switchtec/switchtec.h"
#include <switchtec/utils.h>
#include "suffix.h"

#include <sys/time.h>

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <signal.h>
#include <math.h>

#define WINBORDER  '|', '|', '-', '-', 0, 0, 0, 0
#define WINPORTX 22
#define WINPORTY 16
#define WINPORTSIZE WINPORTY, WINPORTX

#define GUI_INIT_TIME 100000

static WINDOW *mainwin;

struct portloc {
	unsigned startx;
	unsigned starty;
};

static void gui_timer(unsigned duration)
{

	struct itimerval it;

	timerclear(&it.it_interval);
	timerclear(&it.it_value);

	it.it_interval.tv_sec = duration;
	it.it_value.tv_sec = duration;
	setitimer(ITIMER_REAL, &it, NULL);
}

static void cleanup(void)
{
	delwin(mainwin);
	endwin();
	refresh();
}

static void cleanup_and_exit(void)
{
	cleanup();
	exit(0);
}

static void cleanup_and_error(const char *str)
{
	cleanup();
	switchtec_perror(str);
	exit(1);
}

static void gui_handler(int signum)
{

	extern WINDOW *mainwin;

	switch (signum) {

	case SIGTERM:
	case SIGINT:
	case SIGALRM:
		cleanup_and_exit();
	}
}

static int reset_signal;
static void sigusr1_handler(int sig)
{
	reset_signal = 1;
}

static void gui_signals(void)
{

	struct sigaction sa;

	sa.sa_handler = gui_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGALRM, &sa, NULL);
	signal(SIGUSR1, sigusr1_handler);
}

  /* Generate a port based string for the port windows */

static void portid_str(char *str, struct switchtec_port_id *port_id)
{
	sprintf(str, "%s (%d-%d-%d-%d)", port_id->upstream ? "^ " : "v ",
		port_id->phys_id, port_id->partition, port_id->stack,
		port_id->stk_id);
}

  /* Determine positioning for the port windows. Upstream ports at the
   * top, down-stream ports at the bottom. */

static void get_portlocs(struct portloc *portlocs, unsigned all_ports,
			 struct switchtec_status *status, int numports)
{
	unsigned p, nup, ndown, iup, idown;
	nup = ndown = iup = idown = 0;
	for (p = 0; p < numports; p++) {
		if (!all_ports && !status[p].link_up)
			continue;
		if (status[p].port.upstream)
			nup++;
		else
			ndown++;
	}

	for (p = 0; p < numports; p++) {
		if (!all_ports && !status[p].link_up)
			continue;
		if (status[p].port.upstream) {
			portlocs[p].startx =
			    (iup + 1) * COLS / (nup + 1) - WINPORTY / 2;
			portlocs[p].starty = 1;
			iup++;
		} else {
			portlocs[p].startx =
			    (idown + 1) * COLS / (ndown + 1) - WINPORTY / 2;
			portlocs[p].starty = LINES - WINPORTY - 1;
			idown++;
		}
	}

}

static void gui_printbw(char *str, const char *prefix, double val,
			const char *suf)
{
	if (isinf(val) || isnan(val))
		sprintf(str, "%s: --", prefix);
	else
		sprintf(str, "%s: %-.3g %sB/s", prefix, val, suf);
}

struct portstats {
	double tot_val_ingress;
	double tot_val_egress;
	double bw_rate_ingress;
	double bw_rate_egress;
};

static void gui_portcalc(struct switchtec_bwcntr_res *after,
			 struct switchtec_bwcntr_res *before,
			 struct portstats *stats)
{
	uint64_t ingress_tot, egress_tot;
	struct switchtec_bwcntr_res this;

	memcpy(&this, after, sizeof(this));

	stats->tot_val_ingress = switchtec_bwcntr_tot(&after->ingress);
	stats->tot_val_egress = switchtec_bwcntr_tot(&after->egress);

	switchtec_bwcntr_sub(&this, before);
	ingress_tot = switchtec_bwcntr_tot(&this.ingress);
	egress_tot = switchtec_bwcntr_tot(&this.egress);

	stats->bw_rate_ingress = ingress_tot / (this.time_us * 1e-6);
	stats->bw_rate_egress = egress_tot / (this.time_us * 1e-6);
}

  /* Draw a window for the port. */

static WINDOW *gui_portwin(struct portloc *portlocs,
			   struct switchtec_status *s, struct portstats *stats)
{
	WINDOW *portwin;
	char str[256];
	const char *tot_suf, *bw_suf;

	portwin = newwin(WINPORTSIZE, portlocs->starty, portlocs->startx);
	wborder(portwin, WINBORDER);
	portid_str(&str[0], &s->port);
	mvwaddstr(portwin, 1, 1, &str[0]);

	sprintf(&str[0], "Link %s", s->link_up ? "UP" : "DOWN");
	mvwaddstr(portwin, 2, 1, &str[0]);

	sprintf(&str[0], "%s-x%d", s->ltssm_str, s->cfg_lnk_width);
	mvwaddstr(portwin, 3, 1, &str[0]);
	if (!s->link_up)
		goto out;

	sprintf(&str[0], "x%d-Gen%d - %g GT/s",
		s->neg_lnk_width, s->link_rate,
		switchtec_gen_transfers[s->link_rate]);
	mvwaddstr(portwin, 4, 1, &str[0]);

	if (s->vendor_id && s->device_id) {
		snprintf(str, sizeof(str), "%04x:%04x", s->vendor_id,
			 s->device_id);
		mvwaddstr(portwin, 5, 1, &str[0]);
	}

	if (s->class_devices) {
		snprintf(str, sizeof(str), "%s", s->class_devices);
		if (strlen(str) > WINPORTX - 2)
			strcpy(&str[WINPORTX - 6], "...");
		mvwaddstr(portwin, 6, 1, &str[0]);
	}

	tot_suf = suffix_si_get(&stats->tot_val_ingress);
	sprintf(&str[0], "I: %-.3g %sB", stats->tot_val_ingress, tot_suf);
	mvwaddstr(portwin, 8, 1, &str[0]);

	tot_suf = suffix_si_get(&stats->tot_val_egress);
	sprintf(&str[0], "E: %-.3g %sB", stats->tot_val_egress, tot_suf);
	mvwaddstr(portwin, 9, 1, &str[0]);

	bw_suf = suffix_si_get(&stats->bw_rate_ingress);
	gui_printbw(str, "I", stats->bw_rate_ingress, bw_suf);
	mvwaddstr(portwin, 11, 1, &str[0]);

	bw_suf = suffix_si_get(&stats->bw_rate_egress);
	gui_printbw(str, "E", stats->bw_rate_egress, bw_suf);
	mvwaddstr(portwin, 12, 1, &str[0]);

 out:
	wrefresh(portwin);
	return portwin;
}

static int gui_winports(struct switchtec_dev *dev, unsigned all_ports,
			struct switchtec_bwcntr_res *bw_data,
			unsigned reset_cntrs)
{
	int ret, p, numports, port_ids[SWITCHTEC_MAX_PORTS];
	struct switchtec_status *status;
	struct switchtec_bwcntr_res bw_data_new[SWITCHTEC_MAX_PORTS];

	ret = switchtec_status(dev, &status);
	if (errno == EINTR) {
		errno = 0;
		return -1;
	} else if (ret < 0) {
		cleanup_and_error("status");
	}
	numports = ret;

	ret = switchtec_get_devices(dev, status, numports);
	if (ret < 0)
		cleanup_and_error("get_devices");

	struct portloc portlocs[numports];
	WINDOW *portwins[numports];

	get_portlocs(portlocs, all_ports, status, numports);

	for (p = 0; p < numports; p++)
		port_ids[p] = status[p].port.phys_id;

	if (reset_cntrs) {
		switchtec_bwcntr_many(dev, numports, port_ids, 1,
				      NULL);
		switchtec_bwcntr_many(dev, numports, port_ids, 0,
				      bw_data);
	}

	ret = switchtec_bwcntr_many(dev, numports, port_ids, 0, bw_data_new);
	if (errno == EINTR) {
		errno = 0;
		goto free_and_return;
	} else if (ret < 0) {
		cleanup_and_error("bwcntr");
	}

	for (p = 0; p < numports; p++) {
		struct switchtec_status *s = &status[p];
		struct portstats stats;

		if (all_ports || s->link_up) {
			gui_portcalc(&bw_data_new[p], &bw_data[p], &stats);
			portwins[p] = gui_portwin(&portlocs[p], s, &stats);
			wrefresh(portwins[p]);
		}
	}

	memcpy(bw_data, bw_data_new, SWITCHTEC_MAX_PORTS *
	       sizeof(struct switchtec_bwcntr_res));
	ret = 0;

free_and_return:
	switchtec_status_free(status, numports);
	return ret;
}

static int gui_init(struct switchtec_dev *dev, unsigned reset,
		    struct switchtec_bwcntr_res *bw_data)
{
	int ret, p, numports, port_ids[SWITCHTEC_MAX_PORTS];
	struct switchtec_status *status;

	ret = switchtec_status(dev, &status);
	if (ret < 0)
		cleanup_and_error("status");
	numports = ret;

	for (p = 0; p < numports; p++)
		port_ids[p] = status[p].port.phys_id;

	if (reset)
		ret = switchtec_bwcntr_many(dev, numports, port_ids, 1, NULL);
	else
		ret = switchtec_bwcntr_many(dev, numports, port_ids, 0,
					    bw_data);

	switchtec_status_free(status, numports);
	return ret;
}

/*
 * Function to handle keypresses when in GUI mode. For now only 'r'
 * has any significance (resets counters).
 */

static unsigned gui_keypress()
{
	int ch = getch();

	switch (ch) {
	case 'r':
		return 1;
	case 'q':
		cleanup_and_exit();
	case KEY_RESIZE:
		clear();
		wborder(mainwin, WINBORDER);
		wrefresh(mainwin);
		refresh();
		break;
	}
	return 0;
}

  /* Main GUI window. */

int gui_main(struct switchtec_dev *dev, unsigned all_ports, unsigned reset,
	     unsigned refresh, int duration)
{

	if ((mainwin = initscr()) == NULL) {
		fprintf(stderr, "Error initialising ncurses.\n");
		exit(EXIT_FAILURE);
	}
	wborder(mainwin, WINBORDER);
	wrefresh(mainwin);
	gui_signals();
	if (duration >= 0)
		gui_timer(duration);
	nodelay(stdscr, TRUE);
	noecho();
	cbreak();

	int ret;
	struct switchtec_bwcntr_res bw_data[SWITCHTEC_MAX_PORTS] = { {0} };

	ret = gui_init(dev, reset, bw_data);
	if (ret < 0) {
		switchtec_perror("gui_main");
		return ret;
	}
	usleep(GUI_INIT_TIME);

	while (1) {
		int do_reset;

		do_reset = gui_keypress();
		do_reset |= reset_signal;
		ret = gui_winports(dev, all_ports, bw_data, do_reset);
		sleep(refresh);

		if (!ret && do_reset)
			reset_signal = 0;
	}

	return 0;
}
