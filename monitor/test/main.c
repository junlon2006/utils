#include "uni_monitor.h"

#include <stdio.h>
#include <math.h>

#define LOOP_TIMES   0x7fffff

float InvSqrt(float x) {
  float xhalf = 0.5f * x;
  int i = *(int *)&x;
  i = 0x5f3759df - (i>>1);
  x = *(float *)&i;
  x = x * (1.5f - xhalf * x * x);
  return x;
}

static int _func() {
  static int index = 0;
  float f = 0.0;
  int i;
  MonitorStepInto();
  for (i = 0; i < LOOP_TIMES; i++) {
    f += (1.0 / InvSqrt(i));
  }
  MonitorStepOut();
  if (i == LOOP_TIMES && index++ == 100)printf("f=%f\n", f);
  return 0;
}

static int __func() {
  static int index = 0;
  float f = 0.0;
  int i;
  MonitorStepInto();
  for (i = 0; i < LOOP_TIMES; i++) {
    f += sqrt(i);
  }
  MonitorStepOut();
  if (i == LOOP_TIMES && index++ == 100)printf("f=%f\n", f);
  return 0;
}

int main() {
  int i = 0xff;
  printf("i=%d\n", i);
  MonitorInitialize();
  while (i--) {
    _func();
    __func();
  }
  MonitorPrintStatus();
  MonitorFinalize();
  return 0;
}
