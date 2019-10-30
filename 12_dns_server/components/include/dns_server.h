#ifndef __DNA_SERVER_H__
#define __DNA_SERVER_H__

#define SIZEOF_DNSANS_HDR    12
#define SIZEOF_DNSANS_HDRQUE 20

#define DNS_SERVER_ID        0x0000
#define DNS_SERVER_FLAGS     0x8180
#define DNS_SERVER_NUMQUE    0x0001
#define DNS_SERVER_ANSRRS    0x0001
#define DNS_SERVER_AUTRRS    0x0000
#define DNS_SERVER_ADDRRS    0x0000
#define TABLENAME "www.xuhong.com"
#define DNS_SERVER_TYPE      0x0001
#define DNS_SERVER_CLASS     0x0001
#define DNS_POINAME          0xC00C
#define DNS_SERVER_ANSTYPE   0x0001
#define DNS_SERVER_ANSTYPEE  0x0001
#define DNS_SERVER_DATALEN   0x0000
#define DNS_SERVER_ANSTIME   0x003c0004
#define DNS_SERVER_ADRESS    0xc0a80401
void dns_server_send(void);
void get_dns_request(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
void my_udp_init(void);

#endif