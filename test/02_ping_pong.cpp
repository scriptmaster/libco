#include "test_common.hpp"

static cothread_t host_thread;
static cothread_t ping_thread;
static cothread_t pong_thread;
static int sequence[6];
static int n = 0;

static void ping_entry() {
  sequence[n++] = 1;
  co_switch(pong_thread);
  sequence[n++] = 3;
  co_switch(host_thread);
  for(;;) co_switch(host_thread);
}

static void pong_entry() {
  sequence[n++] = 2;
  co_switch(ping_thread);
  sequence[n++] = 4;
  co_switch(host_thread);
  for(;;) co_switch(host_thread);
}

int main() {
  host_thread = co_active();
  ping_thread = co_create(kStackSize, ping_entry);
  pong_thread = co_create(kStackSize, pong_entry);
  TEST_ASSERT(ping_thread != 0 && pong_thread != 0, "co_create failed");

  co_switch(ping_thread);
  co_switch(pong_thread);

  TEST_ASSERT(n == 4, "unexpected sequence length");
  TEST_ASSERT(sequence[0] == 1 && sequence[1] == 2 && sequence[2] == 3 && sequence[3] == 4, "unexpected ping/pong order");

  co_delete(ping_thread);
  co_delete(pong_thread);
  std::puts("02_ping_pong: ok");
  return 0;
}
