#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

volatile int *test_address;

void signal_handler(int signum) {
  printf("Caught signal %d\n", signum);
  exit(signum);
}

int main() {
  signal(SIGSEGV, signal_handler);

  // Allocate memory
  test_address = malloc(sizeof(int));
  if (!test_address) {
    perror("malloc");
    return 1;
  }

  // Print the address for reference
  printf("Allocated memory at address: %p\n", (void *)test_address);

  // Write to the test_address to trigger the watchpoint
  *test_address = 42;

  printf("Value at test_address: %d\n", *test_address);
  getchar();
  *test_address = 6;
  printf("Value at test_address: %d\n", *test_address);
  // Free allocated memory
  getchar();
  free((void *)test_address);

  return 0;
}
