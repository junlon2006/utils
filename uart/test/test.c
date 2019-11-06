
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "../inc/uni_uart.h"

static int _recv_uart_frame(char *buffer, int len) {
  printf("recv a frame.\n");
}

int main(int argc, char const *argv[]) {
  UniUartConfig config;
  strcpy(config.device, "/dev/ttyO0");
  config.speed = B9600;

  /* step 1. register on frame hook */
  UniUartRegisterRecvFrameHandler(_recv_uart_frame);

  /* step 2. init */
  UniUartInitialize(&config);

  /* step 3. do something */
  while(1) usleep(10000);

  /* step 4. finalize*/
  UniUartFinalize();

  /* step 5. unregister on frame hook*/
  UniUartUnregisterRecvFrameHandler();
  return 0;
}
