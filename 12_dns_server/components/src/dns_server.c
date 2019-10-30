/*
 * @Author: your name
 * @Date: 2019-10-30 10:22:47
 * @LastEditTime: 2019-10-30 10:56:13
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-idf\mine\ESP32_DNS_Server\components\src\dns_server.c
 */
#include <string.h>
#include <sys/socket.h>
#include "esp_system.h"
#include "esp_log.h"
#include "tcpip_adapter.h"
#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/ip.h"
#include "lwip/udp.h"
#include "lwip/def.h"
#include "lwip/memp.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip6_addr.h"
#include "lwip/api.h"
#include "dns_server.h"

#define TAG "lwip_udp"

uint16_t txid = 0;
uint16_t nquestions = 0;
uint16_t nanswers = 0;
struct udp_pcb *upcb1;
const ip_addr_t *addr1;
u16_t port1;

struct dns_ans_hdr
{
    PACK_STRUCT_FIELD(uint16_t id);
    PACK_STRUCT_FIELD(uint16_t flag);
    PACK_STRUCT_FIELD(uint16_t numquestions);
    PACK_STRUCT_FIELD(uint16_t numanswers);
    PACK_STRUCT_FIELD(uint16_t numauthrr);
    PACK_STRUCT_FIELD(uint16_t numextrarr);
} PACK_STRUCT_STRUCT;

struct dns_ans_ans
{
    uint16_t typ;
    uint16_t cls;
    uint16_t point;
    uint16_t antyp;
    uint16_t antypp;
    uint16_t len;
    uint32_t time;
    uint32_t addr;
};

struct dns_table_entry
{
    uint16_t txid;
    uint16_t flags;
    uint16_t numque;
    uint16_t ansrrs;
    uint16_t autrrs;
    uint16_t addrrs;
    char name[256];
    uint16_t type;
    uint16_t class;
    uint16_t poiname;
    uint16_t anstype;
    uint16_t anstypee;
    uint16_t datalen;
    uint32_t anstime;
    uint32_t adress;
};

void dns_server_send(void)
{
    struct pbuf *rp = NULL;
    struct dns_ans_hdr hdr;
    struct dns_ans_ans qry;
    uint8_t n;
    uint16_t query_idx, copy_len;
    const char *hostname, *hostname_part;
    struct dns_table_entry dns_server_table = {
        .txid = DNS_SERVER_ID,
        .flags = DNS_SERVER_FLAGS,
        .numque = DNS_SERVER_NUMQUE,
        .ansrrs = DNS_SERVER_ANSRRS,
        .autrrs = DNS_SERVER_AUTRRS,
        .addrrs = DNS_SERVER_ADDRRS,
        .name = {0},
        .type = DNS_SERVER_TYPE,
        .class = DNS_SERVER_CLASS,
        .poiname = DNS_POINAME,
        .anstype = DNS_SERVER_ANSTYPE,
        .anstypee = DNS_SERVER_ANSTYPEE,
        .datalen = DNS_SERVER_DATALEN,
        .anstime = DNS_SERVER_ANSTIME,
        .adress = DNS_SERVER_ADRESS};
    strcpy(dns_server_table.name, TABLENAME);
    struct dns_table_entry *entry = &dns_server_table;

    rp = pbuf_alloc(PBUF_TRANSPORT, 51, PBUF_RAM);
    if (rp != NULL)
    {
        memset(&hdr, 0, SIZEOF_DNSANS_HDR);
        /* fill dns_ans header */
        hdr.id = htons(txid);
        hdr.flag = htons(entry->flags);
        hdr.numquestions = htons(entry->numque);
        hdr.numanswers = htons(entry->ansrrs);
        hdr.numauthrr = htons(entry->autrrs);
        hdr.numextrarr = htons(entry->addrrs);
        pbuf_take(rp, &hdr, SIZEOF_DNSANS_HDR);
        /* convert hostname into suitable query format. */
        hostname = entry->name;
        --hostname;
        query_idx = SIZEOF_DNSANS_HDR;
        do
        {
            ++hostname;
            hostname_part = hostname;
            for (n = 0; *hostname != '.' && *hostname != 0; ++hostname)
            {
                ++n;
            }
            copy_len = (u16_t)(hostname - hostname_part);
            pbuf_put_at(rp, query_idx, n);
            pbuf_take_at(rp, hostname_part, copy_len, query_idx + 1);
            query_idx += n + 1;
        } while (*hostname != 0);
        pbuf_put_at(rp, query_idx, 0);
        query_idx++;
        /* fill dns ans */
        qry.typ = htons(entry->type);
        qry.cls = htons(entry->class);
        qry.point = htons(entry->poiname);
        qry.antyp = htons(entry->anstype);
        qry.antypp = htons(entry->anstypee);
        qry.len = htons(entry->datalen);
        qry.time = htonl(entry->anstime);
        qry.addr = htonl(entry->adress);
        printf("the query_idx is %d\n", query_idx);
        printf("the qry.addr is %02X\n", qry.addr);
        pbuf_take_at(rp, &qry, SIZEOF_DNSANS_HDRQUE, query_idx);

        udp_sendto(upcb1, rp, addr1, port1);
        //      pbuf_free(rp);
    }
}

void get_dns_request(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    printf("get the massage from sat\n");
    struct dns_ans_hdr hdr;
    upcb1 = upcb;
    addr1 = addr;
    port1 = port;
    if (p->tot_len < (SIZEOF_DNSANS_HDR + SIZEOF_DNSANS_HDRQUE))
    {
        LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: pbuf too small\n"));
        printf("dns_recv: pbuf too small\n");
        /* free pbuf and return */
    }
    else
    {
        pbuf_copy_partial(p, &hdr, SIZEOF_DNSANS_HDR, 0);
        txid = ntohs(hdr.id);
        nquestions = ntohs(hdr.numquestions);
    }
    printf("the length of q: %d\n", p->tot_len);
    printf("the txid is: %02X and the questions number is %02X\n", txid, nquestions);
    pbuf_free(p); //check this
    dns_server_send();
}

void my_udp_init(void)
{
    struct udp_pcb *upcb;
    err_t err;

    upcb = udp_new();
    err = udp_bind(upcb, IP_ADDR_ANY, 53);
    if (err == ERR_OK)
    {
        udp_recv(upcb, get_dns_request, NULL);
    }
    else
    {
        udp_remove(upcb);
    }
}