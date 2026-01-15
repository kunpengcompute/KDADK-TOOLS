/*
Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 */
#ifndef KDADK_TYPE_H
#define KDADK_TYPE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pcap.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <libgen.h>
#include <unistd.h>
#include <linux/limits.h>
#include "kdadk_feature_extract.h"

typedef struct {
    int *classes;
    int num_classes;
    double *precision;
    double *recall;
    double *f1_score;
    int *support;
    double accuracy;
    double macro_avg_precision;
    double macro_avg_recall;
    double macro_avg_f1;
    double weighted_avg_precision;
    double weighted_avg_recall;
    double weighted_avg_f1;
} ClassificationReport;

// 性能统计结构
typedef struct {
    double pcap_read_time;           // PCAP读取时间
    double feature_extraction_time;  // 特征提取时间
    double inference_time;           // 推理时间
    double file_io_time;             // 文件IO时间
    double total_time;               // 特征提取 + 推理 + 文件IO 总时间
    double end_to_end_time;          // 端到端时间（含PCAP读取及内存分配）
    uint64_t total_bytes;            // 总字节数
    uint64_t total_packets;          // 总包数
} PerformanceStats;

// 过滤后的特征数据（保留原始特征，只过滤行）
typedef struct {
    feature_vector *vectors;  // 过滤后保留的特征向量（原始特征）
    uint32_t count;           // 过滤后的数量
    uint32_t capacity;        // 容量
    uint32_t filtered_count;  // 被过滤掉的数量
} filtered_features;

#endif