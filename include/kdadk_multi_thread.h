/*
Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 */

#ifndef KDADK_MULTI_THREAD_H
#define KDADK_MULTI_THREAD_H

#include "kdadk_type.h"
#include "kdadk_feature_extract.h"
#include "kdadk.h"
#include <pthread.h>
#include <queue>
#include <vector>
#include <atomic>

// 前向声明
template <typename T, size_t Capacity>
class LockFreeCircularQueue;

// 数据包结构体
typedef struct {
    u_char *packet;
    int packet_len;
    struct timeval ts;
    feature_vector *features;
    rawbow *raw_payload;
    int *has_feature;
} multi_thread_packet_info;

#define BATCH_SIZE 64  // 每 64 个包进行一次跨线程通信

struct packet_batch {
    multi_thread_packet_info pkts[BATCH_SIZE];
    size_t count = 0;
};

// 数据包节点结构体
typedef struct packet_node_t {
    multi_thread_packet_info info;
    struct packet_node_t *next;
} packet_node;

// 工作线程结构体
typedef struct alignas(64) worker_thread {
    int thread_id;
    pthread_t thread;
    feature_extractor *extractor;
    // 主线程暂存区（满 BATCH_SIZE 后再提交）
    packet_batch staging_batch;
    // 队列类型改为传递 batch
    LockFreeCircularQueue<packet_batch, 256> *queue; // 无锁循环队列
    LockFreeCircularQueue<packet_batch, 256> *recycle_queue; // 回收队列
    std::atomic<bool> running;
    feature_vector_list features;
    rawbow_list rawbows;
} worker_thread;

// 多线程特征提取器结构体
typedef struct {
    int thread_count;
    worker_thread *workers;
    pthread_mutex_t result_mutex;
    feature_vector_list all_features;
    rawbow_list all_rawbows;
    domain_name_hash *shared_domain_table; // 共享域名表指针
    bool enable_shared_domain_table; // 是否启用共享域名表
} multi_thread_extractor;

/**
 * 初始化多线程特征提取器
 * @param thread_count 线程数
 * @param dlt 数据链路类型
 * @return 多线程特征提取器实例
 */
multi_thread_extractor *multi_extractor_init(int thread_count, int dlt);

/**
 * 处理数据包
 * @param extractor 多线程特征提取器实例
 * @param packet 数据包
 * @param packet_len 数据包长度
 * @param ts 时间戳
 * @param features 特征向量
 * @param has_feature 是否有特征
 * @return 处理结果
 */
int multi_process_packet(multi_thread_extractor *extractor, const u_char *packet, 
                        int packet_len, const struct timeval *ts, 
                        feature_vector *features, int *has_feature);

/**
 * 处理数据包（提取特征向量和 Rawbow）
 * @param extractor 多线程特征提取器实例
 * @param packet 数据包
 * @param packet_len 数据包长度
 * @param ts 时间戳
 * @param features 特征向量
 * @param raw_payload Rawbow 数据
 * @param has_feature 是否有特征
 * @return 处理结果
 */
int multi_process_packet_with_rawbow(multi_thread_extractor *extractor, const u_char *packet, 
                                   int packet_len, const struct timeval *ts, 
                                   feature_vector *features, rawbow *raw_payload, int *has_feature);

/**
 * 完成特征提取
 * @param extractor 多线程特征提取器实例
 * @param features 特征向量列表
 * @return 处理结果
 */
int multi_extractor_finalize(multi_thread_extractor *extractor, feature_vector_list *features);

/**
 * 完成特征提取（提取特征向量和 Rawbow）
 * @param extractor 多线程特征提取器实例
 * @param features 特征向量列表
 * @param rawbows Rawbow 列表
 * @return 处理结果
 */
int multi_extractor_finalize_with_rawbow(multi_thread_extractor *extractor, feature_vector_list *features, 
                                       rawbow_list *rawbows);

/**
 * 销毁多线程特征提取器
 * @param extractor 多线程特征提取器实例
 */
void multi_extractor_destroy(multi_thread_extractor *extractor);

#endif // KDADK_MULTI_THREAD_H