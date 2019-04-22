#include "uni_dns_parse.h"
#include "uni_log.h"
#include <unistd.h>

#define MAIN_TAG  "main"

int main() {
  uint64_t ip;
  DnsParseInit();
  while (1) {
    ip = DnsParseByDomain("www.baidu.com");
    LOGT(MAIN_TAG, "ip=%x", ip);
    ip = DnsParseByDomain("www.github.com");
    LOGT(MAIN_TAG, "ip=%x", ip);
    sleep(1);
  }
  DnsParseFinal();
  return 0;
}
