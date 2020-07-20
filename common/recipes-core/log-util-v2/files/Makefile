# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

all: log-util

TEST_SRCS := $(wildcard tests/*.cpp)
COMMON_SRCS := log-util.cpp rsyslogd.cpp selformat.cpp selstream.cpp
COMMON_OBJS := ${COMMON_SRCS:.cpp=.o}
TEST_OBJS := ${TEST_SRCS:.cpp=.o}
SRCS=$(COMMON_SRCS) $(TEST_SRCS) main.cpp

CXXFLAGS += -std=c++17 -Wall -Werror -g -I.
LDFLAGS += -lpal

log-util: $(COMMON_OBJS) main.o
	$(CXX) -pthread -o log-util $^ $(LDFLAGS)

log-util-test: $(COMMON_OBJS) $(TEST_OBJS)
	$(CXX) -pthread -o log-util-test $^ $(LDFLAGS) -lgtest -lgmock -lgtest_main

$(SRCS:.cpp=.d):%.d:%.cpp
	$(CXX) $(CXXFLAGS) $< >$@

.PHONY: clean

clean:
	rm -rf *.o tests/*.o log-util log-util-test
