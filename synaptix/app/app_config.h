#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* DEVICE IP    */
#define IP4_BYTE1               192
#define IP4_BYTE2               168
#define IP4_BYTE3               4
#define IP4_BYTE4               1

#define IP4_SUBNET_MASK_BYTE1   255
#define IP4_SUBNET_MASK_BYTE2   255
#define IP4_SUBNET_MASK_BYTE3   255
#define IP4_SUBNET_MASK_BYTE4   0

#define IP4_GW_BYTE1            192
#define IP4_GW_BYTE2            168
#define IP4_GW_BYTE3            4
#define IP4_GW_BYTE4            1

#define MAC_ADDR_BYTE1          0x02
#define MAC_ADDR_BYTE2          0x02
#define MAC_ADDR_BYTE3          0x84
#define MAC_ADDR_BYTE4          0x6A
#define MAC_ADDR_BYTE5          0x96
#define MAC_ADDR_BYTE6          0x00

/*  ROUTER IP    */
#define IP4_ROUTER_BYTE1        192
#define IP4_ROUTER_BYTE2        168
#define IP4_ROUTER_BYTE3        4
#define IP4_ROUTER_BYTE4        1

#define LISTEN_PORT             67

#define ENTRY_TIMEOUT           24 * 60 * 60

#define IP4_DNS_BYTE1           IP4_GW_BYTE1
#define IP4_DNS_BYTE2           IP4_GW_BYTE2
#define IP4_DNS_BYTE3           IP4_GW_BYTE3
#define IP4_DNS_BYTE4           IP4_GW_BYTE4


#ifdef __cplusplus
}
#endif

#endif // APP_CONFIG_H