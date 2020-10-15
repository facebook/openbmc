/**
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

// Package logger provides a Writer implementation to be used for log.SetOutput
// to enable logging to both stderr and syslog.
package logger

import (
	"fmt"
	"io"
	"log"
	"log/syslog"
	"os"
	"strings"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
)

// LogWriter is an implementation of io.Writer that will call write
// to all streams defined in Streams.
type LogWriter struct {
	Streams []io.Writer
}

// InitCustomLogger sets CustomLogger to contain the stderr stream and tries to acquire the
// syslog stream.
var InitCustomLogger = func() {
	// stderr
	streams := []io.Writer{
		os.Stderr,
	}

	// syslog
	syslogWriter, err := getSyslogWriter()
	if err != nil {
		log.Printf("Unable to get syslog: %v", err)
	} else {
		streams = append(streams, syslogWriter)
	}

	log.SetOutput(LogWriter{
		Streams: streams,
	})
}

// InitCustomLoggerStderrOnly sets CustomLogger to only contain the stderr stream.
// This is used for situations in which code may kill rsyslogd (e.g. fuser -km).
var InitCustomLoggerStderrOnly = func() {
	log.SetOutput(LogWriter{
		Streams: []io.Writer{os.Stderr},
	})
}

// LogWriter implements io.Writer
func (w LogWriter) Write(p []byte) (n int, err error) {
	for _, stream := range w.Streams {
		n, err = stream.Write(p)
		if err != nil {
			// don't panic, just log to stderr that this has failed.
			// there are cases of syslog delivery failing and causing the whole upgrade to fail.
			println(fmt.Sprintf("%v", err))
		}
	}
	return
}

// getSyslogWriter gets the syslog writer. If syslog.New fails,
// call startSyslog and try up to a maximum of 3 times
func getSyslogWriter() (syslogWriter io.Writer, err error) {
	prefixTag := fmt.Sprintf("[flashy|%v]", strings.Join(os.Args, " "))
	for i := 0; i < 3; i++ {
		syslogWriter, err = syslog.New(syslog.LOG_INFO, prefixTag)
		if err != nil {
			startSyslog()
		} else {
			return
		}
	}
	return
}

// startSyslog runs the init script to start rsyslog or BusyBox syslog.
// rsyslog init has the `--oknodo flag` and will reliably return
// 0 if already started (common/recipes-extended/rsyslog/files/initscript).
// However, busybox syslog returns 1 if already started, so we ignore the
// error from RunCommand.
var startSyslog = func() {
	systemdAvail, err := utils.SystemdAvailable()
	if err != nil {
		log.Printf("Unable to decide if systemd is available: %v", err)
		return
	}
	cmd := []string{}
	if systemdAvail {
		cmd = []string{"systemctl", "start", "syslog"}
	} else {
		cmd = []string{"/etc/init.d/syslog", "start"}
	}
	utils.RunCommand(cmd, 30*time.Second)
}
