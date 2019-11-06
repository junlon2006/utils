
#ifndef AVFORMAT_UNI_DNS_PARSE_H
#define AVFORMAT_UNI_DNS_PARSE_H
#include <netdb.h>

void dns_parse_init(const char *config_path);
int dns_parse_by_domain(char *hostname, char *portstr,
                        struct addrinfo *hints, struct addrinfo **ai);
void dns_parse_by_domain_done();

#endif
