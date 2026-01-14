/*
Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 */
#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include <stdint.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "process_feature.h"

#define HASH_BUCKETS 65536
#define DLT_EN10MB 1
#define DLT_LINUX_SLL 113

#ifdef __cplusplus
extern "C" {
#endif

#define FEATURES_NUM FEATURE_MAX_SIZE + 1 /* 354 + 1 (flow_id在第0列) */

typedef struct {
    float features[FEATURES_NUM]; /* features[0]是flow_id，features[1-354]是实际特征 */
} feature_vector;

typedef struct {
    feature_vector *vectors;
    uint32_t count;
    uint32_t capacity;
} feature_vector_list;

typedef struct {
    char rawbow[RAWBOW_MAX_LEN];
    uint32_t len;
} rawbow;

typedef struct {
    rawbow *rawbows;
    uint32_t count;
    uint32_t capacity;
} rawbow_list;

typedef struct {
    uint32_t total_packets;   /* 总包数 */
    uint64_t total_bytes;     /* 总字节数 */
    uint32_t decoded_packets; /* 成功解析的包数 */
    uint32_t extracted_flows; /* 已提取的流数 */
} statistics;

typedef struct {
    flow_hash *hash_table;
    domain_name_hash *domain_name_table;
    int link_type;
    int initialized;
    uint64_t next_flow_id; /* 全局唯一的flow_id计数器 */
    statistics stats;      /* 统计信息 */
} feature_extractor;

#define EXTRACTOR_SUCCESS 0
#define EXTRACTOR_ERROR -1

/**
 * @brief 创建并初始化特征提取器
 * @param link_type 链路层类型
 * @return 特征提取器句柄，失败返回NULL
 */
feature_extractor *extractor_init(int link_type);

/**
 * @brief 销毁特征提取器
 * @param extractor 特征提取器句柄
 */
void extractor_destroy(feature_extractor *extractor);

/**
 * @brief 处理单个数据包（仅提取特征向量）
 * @param extractor 特征提取器句柄
 * @param packet 数据包指针
 * @param packet_len 数据包长度
 * @param ts 时间戳
 * @param features 输出特征向量（单个流）
 * @param has_feature 输出标志，表示是否有特征提取（流结束）
 * @return EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败
 */
int process_packet(feature_extractor *extractor, const unsigned char *packet, uint32_t packet_len,
                   const struct timeval *ts, feature_vector *features, int *has_feature);

/**
 * @brief 处理单个数据包（提取特征向量和rawbow）
 * @param extractor 特征提取器句柄
 * @param packet 数据包指针
 * @param packet_len 数据包长度
 * @param ts 时间戳
 * @param features 输出特征向量（单个流）
 * @param rawbows 输出rawbow（单个流）
 * @param has_feature 输出标志，表示是否有特征提取（流结束）
 * @return EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败
 */
int process_packet_with_rawbow(feature_extractor *extractor, const unsigned char *packet, uint32_t packet_len,
                               const struct timeval *ts, feature_vector *features, rawbow *rawbows, int *has_feature);

/**
 * @brief 处理所有剩余的流（仅提取特征向量）
 * @param extractor 特征提取器句柄
 * @param features 输出特征向量列表
 * @return EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败
 */
int extractor_finalize(feature_extractor *extractor, feature_vector_list *features);

/**
 * @brief 处理所有剩余的流（提取特征向量和rawbow）
 * @param extractor 特征提取器句柄
 * @param features 输出特征向量列表
 * @param rawbows 输出rawbow列表
 * @return EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败
 */
int extractor_finalize_with_rawbow(feature_extractor *extractor, feature_vector_list *features, rawbow_list *rawbows);

/**
 * @brief 获取统计信息
 * @param extractor 特征提取器句柄
 * @param stats 输出统计信息
 * @return EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败
 */
int extractor_get_statistics(feature_extractor *extractor, statistics *stats);

/**
 * @brief 重置统计信息
 * @param extractor 特征提取器句柄
 */
void extractor_reset_statistics(feature_extractor *extractor);

#ifdef __cplusplus
}
#endif

#endif /* FEATURE_EXTRACTOR_H */