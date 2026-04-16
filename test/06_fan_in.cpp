#include "test_common.hpp"

static cothread_t host_thread;
static cothread_t left_thread;
static cothread_t right_thread;

static int left_values[3] = {1, 3, 5};
static int right_values[3] = {2, 4, 6};
static int left_i = 0;
static int right_i = 0;
static int output_sum = 0;

static void left_entry() {
  while(left_i < 3) {
    output_sum += left_values[left_i++];
    co_switch(host_thread);
  }
  for(;;) co_switch(host_thread);
}

static void right_entry() {
  while(right_i < 3) {
    output_sum += right_values[right_i++];
    co_switch(host_thread);
  }
  for(;;) co_switch(host_thread);
}

int main() {
  host_thread = co_active();
  left_thread = co_create(kStackSize, left_entry);
  right_thread = co_create(kStackSize, right_entry);
  TEST_ASSERT(left_thread && right_thread, "co_create failed");

  while(left_i < 3 || right_i < 3) {
    if(left_i < 3) co_switch(left_thread);
    if(right_i < 3) co_switch(right_thread);
  }

  TEST_ASSERT(output_sum == 21, "fan-in sum mismatch");
  co_delete(left_thread);
  co_delete(right_thread);
  std::puts("06_fan_in: ok");
  return 0;
}
