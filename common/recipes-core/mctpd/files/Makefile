all: mctpd

CXXFLAGS += -Wall -Werror -fPIC -std=c++17 -Wno-reorder
CPP_SRCS := $(wildcard *.cpp)
CPP_OBJS := ${CPP_SRCS:.cpp=.o}

mctpd: $(CPP_OBJS)
	$(CXX) $(CXXFLAGS) -lrt -std=gnu99 -o $@ $^ $(LDFLAGS)


.PHONY: clean

clean:
	rm -rf *.o mctpd
