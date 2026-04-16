#include "test_common.hpp"

static cothread_t host_thread;
static cothread_t workers[3];
static int values[3] = {2, 3, 5};
static int sum = 0;

static void worker0() { sum += values[0]; co_switch(host_thread); for(;;) co_switch(host_thread); }
static void worker1() { sum += values[1]; co_switch(host_thread); for(;;) co_switch(host_thread); }
static void worker2() { sum += values[2]; co_switch(host_thread); for(;;) co_switch(host_thread); }

int main() {
  host_thread = co_active();
  workers[0] = co_create(kStackSize, worker0);
  workers[1] = co_create(kStackSize, worker1);
  workers[2] = co_create(kStackSize, worker2);
  TEST_ASSERT(workers[0] && workers[1] && workers[2], "co_create failed");

  co_switch(workers[0]);
  co_switch(workers[1]);
  co_switch(workers[2]);

  TEST_ASSERT(sum == 10, "unexpected sum");

  co_delete(workers[0]);
  co_delete(workers[1]);
  co_delete(workers[2]);
  std::puts("03_goroutine_style_spawn: ok");
  return 0;
}
