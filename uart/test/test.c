
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "uni_uart.h"

static int _recv_uart_frame(char *buffer, int len) {
  printf("recv a frame.\n");
}

int main(int argc, char const *argv[]) {
  UartConfig config;
  strcpy(config.device, "/dev/ttyO0");
  config.speed = B9600;

  /* step 1. register on frame hook */
  UartRegisterRecvFrameHandler(_recv_uart_frame);

  /* step 2. init */
  UartInitialize(&config);

  /* step 3. do something */
  while(1) usleep(10000);

  /* step 4. finalize*/
  UartFinalize();

  /* step 5. unregister on frame hook*/
  UartUnregisterRecvFrameHandler();
  return 0;
}
