/*
	Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifndef KDADK_H
#define KDADK_H
#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <vector>

#include "kdadk_type.h"

typedef struct {
uint32_t src_ip;
    uint16_t src_port;
    uint8_t protocol;
    uint32_t dst_ip;
    uint16_t dst_port;
} Tuple;

#define IPV4_TUPLE_SIZE          sizeof(Tuple)

typedef struct {
    uint16_t client_version;
    uint16_t cipher_suites_length;
    uint16_t cipher_suites[256];
    uint16_t extensions_length;
    std::string sni; // SNI 字符串
} tls;

typedef struct {
    uint32_t header_len;
    uint32_t payload_len;
    struct timeval ts;
    uint16_t key_len;
    std::string http_uri;
    std::string http_host;
    std::string http_url;
    uint32_t packet_size;
    Tuple tuple;
    uint8_t tcp_flag;
    uint8_t finish_flag;
    tls tls_info;
} packet_info;

typedef struct {
    std::vector<uint32_t> header_len;
    std::vector<uint32_t> payload_len;
    std::vector<struct timeval> ts;
    std::vector<std::string> http_uris;
    std::vector<std::string> http_hosts;
    std::vector<std::string> http_urls;
    std::vector<tls> tls_infos;
    uint64_t total_size;
    uint32_t packets_cnt;
} flow_raw_data;

#define MAX_RAWBOW_CNT     32

typedef enum {
    CONNECTED = 0,
    CLOSING,
    CLOSED
} flow_status;

typedef struct flow_hash_node_t {
    uint32_t bucket_index;
    struct flow_hash_node_t *link_pre, *link_next;
    flow_raw_data raw_data, reverse_raw_data;
    Tuple tuple;
    std::string src_ip_domain_name;
    std::string dst_ip_domain_name;
    std::string rawbow[MAX_RAWBOW_CNT];
    uint32_t rawbow_cnt;
    flow_status status;
    uint32_t flow_src_ip;
} flow_hash_node;

#endif