#include "uni_event.h"
#include "uni_log.h"

#define MAIN_TAG "main_tag"

int main() {
  EventInit();
  EventTypeRegister("EVENT1");
  EventTypeRegister("EVENT2");
  EventTypeRegister("EVENT3");
  EventTypeRegister("EVENT4");
  EventTypeRegister("EVENT5");
  EventTypeRegister("EVENT6");
  EventTypeUnRegister("EVENT3");
  LOGT(MAIN_TAG, "%s", EventGetStringByType(1));
  LOGT(MAIN_TAG, "%d", EventGetTypeByString("EVENT5"));
  EventTypePrintAll();
  EventFinal();
  return 0;
}
