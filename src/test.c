
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
  int fd;
  void *test_address;
  unsigned long addr;

  test_address = malloc(sizeof(int));
  if (!test_address) {
    perror("malloc");
    return -1;
  }

  *(int *)test_address = 42;
  addr = (unsigned long)test_address;

  fd = open("/sys/kernel/watchpoint/watch_address", O_WRONLY);
  if (fd < 0) {
    perror("open");
    return -1;
  }

  printf("%lx\n", addr);
  close(fd);
  getchar();
  *(int *)test_address = 6; // This should trigger the hardware breakpoint

  free(test_address);
  return 0;
}
