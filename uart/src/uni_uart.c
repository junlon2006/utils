
#include "uni_uart.h"
#include "uni_communication.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>

static int uart_fd = -1;
static char is_running = 0;
static UartFrameData _on_frame = NULL;

static int _set_speed(speed_t *speed) {
  int status;
  struct termios options;
  tcgetattr(uart_fd, &options);
  tcflush(uart_fd, TCIOFLUSH);
  cfsetispeed(&options, *speed);
  cfsetospeed(&options, *speed);
  status = tcsetattr(uart_fd, TCSANOW, &options);
  if (0 != status) {
    return -1;
  }
  tcflush(uart_fd, TCIOFLUSH);
  return 0;
}

static void _set_option(struct termios *options) {
  cfmakeraw(options);          /* 配置为原始模式 */
  options->c_cflag &= ~CSIZE;
  options->c_cflag |= CS8;     /* 8位数据位 */
  options->c_cflag |= PARENB;  /* enable parity */
  options->c_cflag &= ~PARODD; /* 偶校验 */
  options->c_iflag |= INPCK;   /* disable parity checking */
  options->c_cflag &= ~CSTOPB; /* 一个停止位 */
  options->c_cc[VTIME] = 0;    /*设置等待时间*/
  options->c_cc[VMIN] = 0;     /*最小接收字符*/
}

static int _set_parity() {
  struct termios options;
  if (tcgetattr(uart_fd, &options) != 0) {
    return -1;
  }
  _set_option(&options);
  if (tcsetattr(uart_fd, TCSANOW, &options) != 0) {
    return -1;
  }
  tcflush(uart_fd, TCIOFLUSH);
  return 0;
}

static void _protocol_buffer_generate(char read_buffer, char *protocol_buffer,
                                      int protocol_buffer_length) {
  static int index = 0;
  static int length = 0;
  if (protocol_buffer_length <= index) {
    index = 0;
    length = 0;
    printf("protocol buffer overflow.\n");
  }
  /* get frame header */
  if (0 == index && read_buffer == UNI_COMM_SYNC_VALUE) {
    protocol_buffer[index++] = read_buffer;
    return;
  }
  /* get payload length */
  if (7 == index) {
    length = read_buffer;
    printf("length7 = %d\n", length);
  } else if(8 == index) {
    length += (((unsigned short)read_buffer) << 8);
    printf("length8 = %d\n", length);
  }
  /* set protocol header */
  if (index < 9 && 1 <= index) {
    protocol_buffer[index++] = read_buffer;
    goto L_END;
  }
  /* set protocol payload */
  if (9 <= index && 0 < length) {
    protocol_buffer[index++] = read_buffer;
    length--;
  }
L_END:
  /* callback protocol buffer */
  if (9 <= index && 0 == length) {
    if (_on_frame) {
      _on_frame(protocol_buffer, index + 1);
    }
    index = 0;
  }
}

static void _free_all() {
  if (uart_fd) {
    close(uart_fd);
    uart_fd = 0;
  }
}

#define PROTOCOL_BUFFER_MAX_SIZE  (512)
static void *_serial_recv_process(void *arg) {
  char buffer = 0x0;
  char protocol_buf[PROTOCOL_BUFFER_MAX_SIZE] = {0};
  fd_set rfds;
  struct timeval tv;
  int retval;
  int read_len = 0;
  pthread_detach(pthread_self());
  while (is_running) {
    FD_ZERO(&rfds);
    FD_SET(uart_fd, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 80 * 1000;
    retval = select(uart_fd + 1, &rfds, NULL, NULL, &tv);
    if (0 < retval && FD_ISSET(uart_fd, &rfds)) {
      read_len = read(uart_fd, &buffer, sizeof(buffer));
      if (1 == read_len) {
        _protocol_buffer_generate(buffer, protocol_buf, PROTOCOL_BUFFER_MAX_SIZE);
      }
    }
  }
  _free_all();
  return NULL;
}

static int _serial_receive_handler_thread_create() {
  pthread_t uart_receive_pid;
  int ret = pthread_create(&uart_receive_pid, NULL, _serial_recv_process, NULL);
  if (ret != 0) {
    printf("uni_receive_serial pthread_create fail!\n");
  }
  return ret;
}

int UartRegisterRecvFrameHandler(UartFrameData handler) {
  _on_frame = handler;
  return 0;
}

int UartUnregisterRecvFrameHandler() {
  return 0;
}

int UartInitialize(UartConfig *config) {
  int flags = O_RDWR | O_NOCTTY | O_NDELAY;
  uart_fd = open(config->device, flags);
  if (-1 == uart_fd) {
    printf("%s%d: open %s failed\n", __FUNCTION__, __LINE__, config->device);
    return -1;
  }
  if (0 != _set_speed(&config->speed)) {
    printf("%s%d: set speed failed\n", __FUNCTION__, __LINE__);
    return -1;
  }
  if (0 != _set_parity()) {
    printf("%s%d: set parity failed\n", __FUNCTION__, __LINE__);
    return -1;
  }
  is_running = 1;
  CommProtocolRegisterWriteHandler(UartWrite);
  _serial_receive_handler_thread_create();
  return 0;
}

int UartFinalize() {
  is_running = 0;
  return 0;
}

int UartWrite(char *buf, int len) {
  return write(uart_fd, buf, len);
}
