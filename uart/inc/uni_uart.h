#ifndef _UTILS_UNI_UART_H_
#define _UTILS_UNI_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <termios.h>

#define UNI_UART_DEVICE_NAME_MAX  (64)

typedef int (*UartFrameData)(char *buf, int len);

typedef struct {
  char    device[UNI_UART_DEVICE_NAME_MAX];
  speed_t speed;
} UartConfig;

int UartRegisterRecvFrameHandler(UartFrameData handler);
int UartUnregisterRecvFrameHandler();
int UartInitialize(UartConfig *config);
int UartFinalize();
int UartWrite(char *buf, int len);

#ifdef __cplusplus
}
#endif
#endif

