CXX ?= c++
CC ?= cc
CXXFLAGS ?= -O2 -std=c++11 -I..
CFLAGS ?= -O2 -std=c89 -Wdeclaration-after-statement -Werror -I..
LDFLAGS ?=

TESTS = \
  01_hello_world \
  02_ping_pong \
  03_goroutine_style_spawn \
  04_round_robin_scheduler \
  05_producer_consumer \
  06_fan_in \
  07_tcp_echo \
  08_tcp_two_clients \
  09_udp_echo \
  10_udp_batch \
  11_http_get \
  12_http_two_clients

all: $(TESTS)

libco.o: ../libco.c ../libco.h ../settings.h
	$(CC) $(CFLAGS) -c ../libco.c -o libco.o

%: %.cpp libco.o test_common.hpp
	$(CXX) $(CXXFLAGS) -o $@ $< libco.o $(LDFLAGS)

run: all
	@set -e; for t in $(TESTS); do echo "==> $$t"; ./$$t; done

clean:
	rm -f *.o $(TESTS)

.PHONY: all run clean
