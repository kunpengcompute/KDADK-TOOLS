/*
Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 */
#include "online_inference.h"
#include "kdadk_file_writer.h"
#include <signal.h>

static volatile sig_atomic_t g_interrupted = 0;

static void signal_handler(int signum)
{
    if (signum == SIGINT) {
        printf("\n\n收到中断信号 (Ctrl+C)，正在安全退出...\n");
        g_interrupted = 1;
    }
}

static void setup_signal_handler(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}

void expand_features_capacity(feature_vector_list *fv)
{
    fv->capacity *= 2;
    fv->vectors = (feature_vector *)realloc(fv->vectors, fv->capacity * sizeof(feature_vector));
}

int filter_features(const inference_engine *engine, const feature_vector_list *features, filtered_features *filtered)
{
    if (!engine || !features || !filtered) {
        return -1;
    }

    filtered->capacity = features->count;
    filtered->vectors = (feature_vector *)malloc(filtered->capacity * sizeof(feature_vector));
    filtered->count = 0;
    filtered->filtered_count = 0;

    int filter_packets = engine->filter_packets;

    for (uint32_t j = 0; j < features->count; j++) {
        const feature_vector *fv = &features->vectors[j];

        if (fv->features[7] + fv->features[165] >= filter_packets) {
            filtered->vectors[filtered->count++] = *fv;
        } else {
            filtered->filtered_count++;
        }
    }
    return 0;
}

void free_filtered_features(filtered_features *filtered)
{
    if (filtered && filtered->vectors) {
        free(filtered->vectors);
        filtered->vectors = NULL;
        filtered->count = 0;
        filtered->capacity = 0;
    }
}

int inference_mode_single_flow(feature_extractor *extractor, inference_engine *engine, pcap_t *handle,
                               int ground_truth_label, filtered_features *all_filtered_out, int **all_predictions,
                               int **all_labels, int *total_count, PerformanceStats *stats)
{
    struct pcap_pkthdr *header;
    const u_char *packet;

    int capacity = 1000;
    *all_predictions = (int *)malloc(capacity * sizeof(int));
    *all_labels = (int *)malloc(capacity * sizeof(int));
    *total_count = 0;

    all_filtered_out->capacity = 1000;
    all_filtered_out->vectors = (feature_vector *)malloc(all_filtered_out->capacity * sizeof(feature_vector));
    all_filtered_out->count = 0;
    all_filtered_out->filtered_count = 0;

    double extract_time = 0, infer_time = 0, pcap_time = 0;

    while (1) {
        if (g_interrupted)
            break;

        double t0 = get_time_in_seconds();
        int ret = pcap_next_ex(handle, &header, &packet);
        double t1 = get_time_in_seconds();

        if (ret < 0)
            break;  // 文件结束或错误
        if (ret == 0)
            continue;  // 超时，继续

        pcap_time += (t1 - t0);
        stats->total_packets++;
        stats->total_bytes += header->caplen;

        feature_vector features;
        int has_feature = 0;

        if (process_packet(extractor, packet, header->caplen, &header->ts, &features, &has_feature) ==
            EXTRACTOR_SUCCESS) {
            double t2 = get_time_in_seconds();
            extract_time += (t2 - t1);

            if (has_feature) {
                feature_vector_list fv_list = {&features, 1, 1};

                filtered_features filtered = {0};
                if (filter_features(engine, &fv_list, &filtered) == 0 && filtered.count > 0) {
                    if (all_filtered_out->count >= all_filtered_out->capacity) {
                        all_filtered_out->capacity *= 2;
                        all_filtered_out->vectors = (feature_vector *)realloc(
                            all_filtered_out->vectors, all_filtered_out->capacity * sizeof(feature_vector));
                    }
                    all_filtered_out->vectors[all_filtered_out->count++] = filtered.vectors[0];
                    all_filtered_out->filtered_count += filtered.filtered_count;

                    // 统计推理时间
                    inference_result result;
                    result.predictions = (int *)malloc(sizeof(int));
                    result.count = 1;

                    double t3 = get_time_in_seconds();
                    if (inference_predict(engine, &fv_list, &result) == INFERENCE_SUCCESS) {
                        double t4 = get_time_in_seconds();
                        infer_time += (t4 - t3);

                        if (*total_count >= capacity) {
                            capacity *= 2;
                            *all_predictions = (int *)realloc(*all_predictions, capacity * sizeof(int));
                            *all_labels = (int *)realloc(*all_labels, capacity * sizeof(int));
                        }
                        (*all_predictions)[*total_count] = result.predictions[0];
                        (*all_labels)[*total_count] = ground_truth_label;
                        (*total_count)++;
                    }
                    free(result.predictions);
                    free_filtered_features(&filtered);
                } else {
                    all_filtered_out->filtered_count++;
                }
            }
        }
    }

    feature_vector_list remaining = {0};
    remaining.capacity = 20000;
    remaining.vectors = (feature_vector *)malloc(remaining.capacity * sizeof(feature_vector));

    double t1 = get_time_in_seconds();
    if (extractor_finalize(extractor, &remaining) == EXTRACTOR_SUCCESS) {
        double t2 = get_time_in_seconds();
        extract_time += (t2 - t1);

        if (remaining.count > 0) {
            filtered_features filtered = {0};
            if (filter_features(engine, &remaining, &filtered) == 0 && filtered.count > 0) {
                for (uint32_t i = 0; i < filtered.count; i++) {
                    if (all_filtered_out->count >= all_filtered_out->capacity) {
                        all_filtered_out->capacity *= 2;
                        all_filtered_out->vectors = (feature_vector *)realloc(
                            all_filtered_out->vectors, all_filtered_out->capacity * sizeof(feature_vector));
                    }
                    all_filtered_out->vectors[all_filtered_out->count++] = filtered.vectors[i];
                }
                all_filtered_out->filtered_count += filtered.filtered_count;

                inference_result result;
                result.predictions = (int *)malloc(filtered.count * sizeof(int));
                result.count = filtered.count;

                double t3 = get_time_in_seconds();
                if (inference_predict(engine, &remaining, &result) == INFERENCE_SUCCESS) {
                    double t4 = get_time_in_seconds();
                    infer_time += (t4 - t3);

                    for (uint32_t i = 0; i < result.count; i++) {
                        if (*total_count >= capacity) {
                            capacity *= 2;
                            *all_predictions = (int *)realloc(*all_predictions, capacity * sizeof(int));
                            *all_labels = (int *)realloc(*all_labels, capacity * sizeof(int));
                        }
                        (*all_predictions)[*total_count] = result.predictions[i];
                        (*all_labels)[*total_count] = ground_truth_label;
                        (*total_count)++;
                    }
                }
                free(result.predictions);
                free_filtered_features(&filtered);
            }
        }
        free(remaining.vectors);
    }

    stats->pcap_read_time += pcap_time;
    stats->feature_extraction_time += extract_time;
    stats->inference_time += infer_time;

    return 0;
}

int inference_mode_batch(feature_extractor *extractor, inference_engine *engine, pcap_t *handle, int ground_truth_label,
                         filtered_features *all_filtered_out, int **all_predictions, int **all_labels, int *total_count,
                         PerformanceStats *stats)
{
    struct pcap_pkthdr *header;
    const u_char *packet;

    feature_vector_list all_features = {0};
    all_features.capacity = 1000;
    all_features.vectors = (feature_vector *)malloc(all_features.capacity * sizeof(feature_vector));

    double extract_time = 0, pcap_time = 0;

    while (1) {
        if (g_interrupted)
            break;

        double t0 = get_time_in_seconds();
        int ret = pcap_next_ex(handle, &header, &packet);
        double t1 = get_time_in_seconds();

        if (ret < 0)
            break;  // 文件结束或错误
        if (ret == 0)
            continue;  // 超时，继续

        pcap_time += (t1 - t0);
        stats->total_packets++;
        stats->total_bytes += header->caplen;

        feature_vector features;
        int has_feature = 0;

        if (process_packet(extractor, packet, header->caplen, &header->ts, &features, &has_feature) ==
            EXTRACTOR_SUCCESS) {
            double t2 = get_time_in_seconds();
            extract_time += (t2 - t1);

            if (has_feature) {
                if (all_features.count >= all_features.capacity) {
                    expand_features_capacity(&all_features);
                }
                all_features.vectors[all_features.count++] = features;
            }
        }
    }

    feature_vector_list remaining = {0};
    remaining.capacity = 20000;
    remaining.vectors = (feature_vector *)malloc(remaining.capacity * sizeof(feature_vector));

    double t1 = get_time_in_seconds();
    if (extractor_finalize(extractor, &remaining) == EXTRACTOR_SUCCESS) {
        double t2 = get_time_in_seconds();
        extract_time += (t2 - t1);

        for (uint32_t i = 0; i < remaining.count; i++) {
            if (all_features.count >= all_features.capacity) {
                expand_features_capacity(&all_features);
            }
            all_features.vectors[all_features.count++] = remaining.vectors[i];
        }
        free(remaining.vectors);
    }

    stats->pcap_read_time += pcap_time;
    stats->feature_extraction_time += extract_time;

    // 过滤特征
    filtered_features filtered = {0};
    if (filter_features(engine, &all_features, &filtered) == 0 && filtered.count > 0) {
        *all_filtered_out = filtered;

        // 批量推理
        inference_result result;
        result.predictions = (int *)malloc(filtered.count * sizeof(int));
        result.count = filtered.count;

        double t3 = get_time_in_seconds();
        if (inference_predict(engine, &all_features, &result) == INFERENCE_SUCCESS) {
            double t4 = get_time_in_seconds();
            stats->inference_time += (t4 - t3);

            *all_predictions = result.predictions;
            *all_labels = (int *)malloc(result.count * sizeof(int));
            for (uint32_t i = 0; i < result.count; i++) {
                (*all_labels)[i] = ground_truth_label;
            }
            *total_count = result.count;
        } else {
            free(result.predictions);
        }
    } else {
        all_filtered_out->count = 0;
        all_filtered_out->vectors = NULL;
        all_filtered_out->filtered_count = all_features.count;
    }

    free(all_features.vectors);
    return 0;
}

int read_features_from_file(const char *file_path, feature_vector_list *features)
{
    FILE *fp = fopen(file_path, "r");
    if (!fp) {
        fprintf(stderr, "错误: 无法打开文件 %s\n", file_path);
        return -1;
    }

    // 初始化特征列表
    features->capacity = 1000;
    features->vectors = (feature_vector *)malloc(features->capacity * sizeof(feature_vector));
    features->count = 0;

    char line[65536];  // 足够大的缓冲区
    int line_num = 0;

    // 跳过CSV头部
    if (fgets(line, sizeof(line), fp) == NULL) {
        fprintf(stderr, "错误: 文件为空\n");
        fclose(fp);
        return -1;
    }
    line_num++;

    // 读取每一行
    while (fgets(line, sizeof(line), fp) != NULL) {
        line_num++;

        // 扩容
        if (features->count >= features->capacity) {
            features->capacity *= 2;
            features->vectors =
                (feature_vector *)realloc(features->vectors, features->capacity * sizeof(feature_vector));
        }

        feature_vector *fv = &features->vectors[features->count];

        // 解析CSV行
        char *token = strtok(line, ",");
        int col = 0;

        while (token != NULL && col < FEATURES_NUM - 1) {
            fv->features[col] = atof(token);
            token = strtok(NULL, ",");
            col++;
        }

        if (col == FEATURES_NUM - 1) {
            features->count++;
        } else {
            fprintf(stderr, "警告: 第 %d 行特征数量不正确 (期望 %d, 实际 %d)\n", line_num, FEATURES_NUM - 1, col);
        }
    }

    fclose(fp);
    printf("从文件读取了 %u 个特征向量\n", features->count);
    return 0;
}

int inference_mode_file(feature_extractor *extractor, inference_engine *engine, pcap_t *handle, int ground_truth_label,
                        filtered_features *all_filtered_out, int **all_predictions, int **all_labels, int *total_count,
                        PerformanceStats *stats, const char *output_file)
{
    struct pcap_pkthdr *header;
    const u_char *packet;

    char temp_file[1024];
    snprintf(temp_file, sizeof(temp_file), "temp.csv");

    file_writer_config writer_config = {
        .file_path = temp_file, .format = FILE_FORMAT_CSV, .mode = WRITE_MODE_FEATURES_ONLY, .append = 0};
    file_writer *writer = writer_create(&writer_config);
    if (!writer) {
        fprintf(stderr, "错误: 无法创建临时文件写入器\n");
        return -1;
    }

    feature_vector_list all_features = {0};
    all_features.capacity = 1000;
    all_features.vectors = (feature_vector *)malloc(all_features.capacity * sizeof(feature_vector));

    double extract_time = 0, io_time = 0, pcap_time = 0;

    while (1) {
        if (g_interrupted)
            break;

        double t0 = get_time_in_seconds();
        int ret = pcap_next_ex(handle, &header, &packet);
        double t1 = get_time_in_seconds();

        if (ret < 0)
            break;
        if (ret == 0)
            continue;

        pcap_time += (t1 - t0);
        stats->total_packets++;
        stats->total_bytes += header->caplen;

        feature_vector features;
        int has_feature = 0;

        if (process_packet(extractor, packet, header->caplen, &header->ts, &features, &has_feature) ==
            EXTRACTOR_SUCCESS) {
            double t2 = get_time_in_seconds();
            extract_time += (t2 - t1);

            if (has_feature) {
                if (all_features.count >= all_features.capacity) {
                    expand_features_capacity(&all_features);
                }
                all_features.vectors[all_features.count++] = features;
            }
        }
    }

    feature_vector_list remaining = {0};
    remaining.capacity = 20000;
    remaining.vectors = (feature_vector *)malloc(remaining.capacity * sizeof(feature_vector));

    double t1 = get_time_in_seconds();
    if (extractor_finalize(extractor, &remaining) == EXTRACTOR_SUCCESS) {
        double t2 = get_time_in_seconds();
        extract_time += (t2 - t1);

        for (uint32_t i = 0; i < remaining.count; i++) {
            if (all_features.count >= all_features.capacity) {
                expand_features_capacity(&all_features);
            }
            all_features.vectors[all_features.count++] = remaining.vectors[i];
        }
        free(remaining.vectors);
    }

    stats->pcap_read_time += pcap_time;
    stats->feature_extraction_time += extract_time;

    // ========== 写入文件 ==========
    double t3 = get_time_in_seconds();
    if (writer_write_features(writer, &all_features) != WRITER_SUCCESS) {
        fprintf(stderr, "错误: 写入特征失败\n");
    }
    writer_flush(writer);
    writer_destroy(writer);
    double t4 = get_time_in_seconds();
    io_time += (t4 - t3);

    printf("特征已写入临时文件: %s (耗时: %.4f 秒)\n", temp_file, t4 - t3);

    free(all_features.vectors);
    all_features.vectors = NULL;
    all_features.count = 0;

    // ========== 从文件读取特征 ==========
    feature_vector_list loaded_features = {0};
    double t5 = get_time_in_seconds();
    if (read_features_from_file(temp_file, &loaded_features) != 0) {
        fprintf(stderr, "错误: 从文件读取特征失败\n");
        stats->file_io_time += io_time;
        remove(temp_file);
        return -1;
    }
    double t6 = get_time_in_seconds();
    io_time += (t6 - t5);

    printf("从文件读取特征完成 (耗时: %.4f 秒)\n", t6 - t5);

    // 过滤特征
    filtered_features filtered = {0};
    if (filter_features(engine, &loaded_features, &filtered) == 0 && filtered.count > 0) {
        *all_filtered_out = filtered;

        inference_result result;
        result.predictions = (int *)malloc(filtered.count * sizeof(int));
        result.count = filtered.count;

        double t7 = get_time_in_seconds();
        if (inference_predict(engine, &loaded_features, &result) == INFERENCE_SUCCESS) {
            double t8 = get_time_in_seconds();
            stats->inference_time += (t8 - t7);

            *all_predictions = result.predictions;
            *all_labels = (int *)malloc(result.count * sizeof(int));
            for (uint32_t i = 0; i < result.count; i++) {
                (*all_labels)[i] = ground_truth_label;
            }
            *total_count = result.count;
        } else {
            free(result.predictions);
        }
    } else {
        all_filtered_out->count = 0;
        all_filtered_out->vectors = NULL;
        all_filtered_out->filtered_count = loaded_features.count;
    }

    stats->file_io_time += io_time;
    free(loaded_features.vectors);
    remove(temp_file);

    return 0;
}

int online_inference_from_interface(inference_engine *engine, const char *interface, const char *output_file,
                                    file_format format)
{
    printf("\n========== 网口实时抓包模式 ==========\n");
    printf("网口: %s\n", interface);
    if (output_file) {
        printf("输出文件: %s (格式: %s)\n", output_file, format == FILE_FORMAT_CSV ? "CSV" : "JSON");
    } else {
        printf("输出: 仅控制台显示\n");
    }

    // 注册信号处理函数
    setup_signal_handler();

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_live(interface, 65535, 1, 1000, errbuf);
    if (!handle) {
        fprintf(stderr, "错误: 无法打开网口: %s\n", errbuf);
        return -1;
    }

    feature_extractor *extractor = extractor_init(DLT_EN10MB);
    if (!extractor) {
        fprintf(stderr, "错误: 无法初始化特征提取器\n");
        pcap_close(handle);
        return -1;
    }

    PerformanceStats stats = {0};
    double start_time = get_time_in_seconds();

    filtered_features all_filtered = {0};
    all_filtered.capacity = 1000;
    all_filtered.vectors = (feature_vector *)malloc(all_filtered.capacity * sizeof(feature_vector));
    all_filtered.count = 0;
    all_filtered.filtered_count = 0;

    int *all_predictions = NULL;
    int predictions_capacity = 1000;
    int predictions_count = 0;
    all_predictions = (int *)malloc(predictions_capacity * sizeof(int));

    struct pcap_pkthdr *header;
    const u_char *packet;
    int flow_count = 0;

    printf("开始实时抓包 (Ctrl+C 退出)...\n\n");

    while (!g_interrupted) {
        double t0 = get_time_in_seconds();
        int ret = pcap_next_ex(handle, &header, &packet);
        double t1 = get_time_in_seconds();

        if (ret < 0)
            break;  // 错误
        if (ret == 0)
            continue;  // 超时

        stats.pcap_read_time += (t1 - t0);
        stats.total_packets++;
        stats.total_bytes += header->caplen;

        feature_vector features;
        int has_feature = 0;

        if (process_packet(extractor, packet, header->caplen, &header->ts, &features, &has_feature) ==
            EXTRACTOR_SUCCESS) {
            double t2 = get_time_in_seconds();
            stats.feature_extraction_time += (t2 - t1);

            if (has_feature) {
                feature_vector_list fv_list = {&features, 1, 1};

                filtered_features filtered = {0};
                if (filter_features(engine, &fv_list, &filtered) == 0 && filtered.count > 0) {
                    if (all_filtered.count >= all_filtered.capacity) {
                        all_filtered.capacity *= 2;
                        all_filtered.vectors = (feature_vector *)realloc(
                            all_filtered.vectors, all_filtered.capacity * sizeof(feature_vector));
                    }
                    all_filtered.vectors[all_filtered.count++] = filtered.vectors[0];

                    // 推理
                    inference_result result;
                    result.predictions = (int *)malloc(sizeof(int));
                    result.count = 1;

                    double t3 = get_time_in_seconds();
                    if (inference_predict(engine, &fv_list, &result) == INFERENCE_SUCCESS) {
                        double t4 = get_time_in_seconds();
                        stats.inference_time += (t4 - t3);

                        // 保存预测结果
                        if (predictions_count >= predictions_capacity) {
                            predictions_capacity *= 2;
                            all_predictions = (int *)realloc(all_predictions, predictions_capacity * sizeof(int));
                        }
                        all_predictions[predictions_count++] = result.predictions[0];

                        printf("Flow %d: 预测类别 = %d\n", flow_count++, result.predictions[0]);
                    }
                    free(result.predictions);
                    free_filtered_features(&filtered);
                } else {
                    all_filtered.filtered_count++;
                }
            }
        }
    }

    // 处理中断信号后的清理工作
    if (g_interrupted) {
        printf("\n正在处理剩余流...\n");
    }

    feature_vector_list remaining = {0};
    remaining.capacity = 10000;
    remaining.vectors = (feature_vector *)malloc(remaining.capacity * sizeof(feature_vector));

    double t1 = get_time_in_seconds();
    if (extractor_finalize(extractor, &remaining) == EXTRACTOR_SUCCESS) {
        double t2 = get_time_in_seconds();
        stats.feature_extraction_time += (t2 - t1);

        if (remaining.count > 0) {
            printf("处理 %u 个剩余流...\n", remaining.count);

            // 过滤特征
            filtered_features filtered = {0};
            if (filter_features(engine, &remaining, &filtered) == 0 && filtered.count > 0) {
                // 保存过滤后的特征
                for (uint32_t i = 0; i < filtered.count; i++) {
                    if (all_filtered.count >= all_filtered.capacity) {
                        all_filtered.capacity *= 2;
                        all_filtered.vectors = (feature_vector *)realloc(
                            all_filtered.vectors, all_filtered.capacity * sizeof(feature_vector));
                    }
                    all_filtered.vectors[all_filtered.count++] = filtered.vectors[i];
                }
                all_filtered.filtered_count += filtered.filtered_count;

                // 推理
                inference_result result;
                result.predictions = (int *)malloc(filtered.count * sizeof(int));
                result.count = filtered.count;

                double t3 = get_time_in_seconds();
                if (inference_predict(engine, &remaining, &result) == INFERENCE_SUCCESS) {
                    double t4 = get_time_in_seconds();
                    stats.inference_time += (t4 - t3);

                    for (uint32_t i = 0; i < result.count; i++) {
                        if (predictions_count >= predictions_capacity) {
                            predictions_capacity *= 2;
                            all_predictions = (int *)realloc(all_predictions, predictions_capacity * sizeof(int));
                        }
                        all_predictions[predictions_count++] = result.predictions[i];
                        printf("Flow %d: 预测类别 = %d\n", flow_count++, result.predictions[i]);
                    }
                }
                free(result.predictions);
                free_filtered_features(&filtered);
            }
        }
        free(remaining.vectors);
    }

    double end_time = get_time_in_seconds();
    stats.total_time = stats.feature_extraction_time + stats.inference_time;
    stats.end_to_end_time = end_time - start_time;

    // 打印性能统计
    print_performance_stats(&stats);

    printf("\n过滤统计: 保留 %u 个样本, 过滤掉 %u 个样本\n", all_filtered.count, all_filtered.filtered_count);
    printf("总共处理: %d 个流\n", flow_count);

    // 保存结果到文件
    if (output_file && all_filtered.count > 0) {
        printf("\n正在保存结果到文件...\n");

        file_writer_config writer_config = {
            .file_path = output_file, .format = format, .mode = WRITE_MODE_WITH_PREDICTIONS, .append = 0};

        file_writer *writer = writer_create(&writer_config);
        if (writer) {
            feature_vector_list filtered_list = {all_filtered.vectors, all_filtered.count, all_filtered.count};

            double io_start = get_time_in_seconds();
            if (writer_write_features_with_predictions(writer, &filtered_list, all_predictions, predictions_count) ==
                WRITER_SUCCESS) {
                writer_flush(writer);
                double io_end = get_time_in_seconds();
                printf("推理结果已保存到: %s (耗时: %.4f 秒)\n", output_file, io_end - io_start);
            } else {
                fprintf(stderr, "错误: 保存结果失败\n");
            }
            writer_destroy(writer);
        }
    }

    // 清理资源
    free_filtered_features(&all_filtered);
    free(all_predictions);
    pcap_close(handle);
    extractor_destroy(extractor);

    return 0;
}

int online_inference_from_pcap(inference_engine *engine, const char **pcap_files, int num_pcap_files, int mode,
                               char *output_file, file_format format)
{
    printf("\n========== PCAP文件推理模式 ==========\n");
    printf("文件数量: %d\n", num_pcap_files);
    printf("推理模式: %d (0=单flow, 1=批处理, 2=文件)\n", mode);
    if (output_file) {
        printf("输出文件: %s (格式: %s)\n", output_file, format == FILE_FORMAT_CSV ? "CSV" : "JSON");
    }

    PerformanceStats stats = {0};
    double total_start = get_time_in_seconds();

    int *all_predictions = NULL;
    int *all_labels = NULL;
    int total_count = 0;

    filtered_features combined_filtered = {0};
    combined_filtered.capacity = 10000;
    combined_filtered.vectors = (feature_vector *)malloc(combined_filtered.capacity * sizeof(feature_vector));
    combined_filtered.count = 0;
    combined_filtered.filtered_count = 0;

    for (int i = 0; i < num_pcap_files; i++) {
        printf("\n处理文件 %d/%d: %s (Label=%d)\n", i + 1, num_pcap_files, pcap_files[i], i);

        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_t *handle = pcap_open_offline(pcap_files[i], errbuf);
        if (!handle) {
            fprintf(stderr, "错误: 无法打开PCAP文件: %s\n", errbuf);
            continue;
        }

        feature_extractor *extractor = extractor_init(DLT_EN10MB);
        if (!extractor) {
            fprintf(stderr, "错误: 无法初始化特征提取器\n");
            pcap_close(handle);
            continue;
        }

        int *predictions = NULL;
        int *labels = NULL;
        int count = 0;
        filtered_features file_filtered = {0};

        switch (mode) {
            case 0:
                inference_mode_single_flow(extractor, engine, handle, i, &file_filtered, &predictions, &labels, &count,
                                           &stats);
                break;
            case 1:
                inference_mode_batch(extractor, engine, handle, i, &file_filtered, &predictions, &labels, &count,
                                     &stats);
                break;
            case 2:
                inference_mode_file(extractor, engine, handle, i, &file_filtered, &predictions, &labels, &count, &stats,
                                    output_file);
                break;
            default:
                fprintf(stderr, "错误: 未知的推理模式 %d\n", mode);
                break;
        }

        // 合并过滤后的特征
        for (uint32_t j = 0; j < file_filtered.count; j++) {
            if (combined_filtered.count >= combined_filtered.capacity) {
                combined_filtered.capacity *= 2;
                combined_filtered.vectors = (feature_vector *)realloc(
                    combined_filtered.vectors, combined_filtered.capacity * sizeof(feature_vector));
            }
            combined_filtered.vectors[combined_filtered.count++] = file_filtered.vectors[j];
        }
        combined_filtered.filtered_count += file_filtered.filtered_count;
        free_filtered_features(&file_filtered);

        // 合并结果
        if (count > 0) {
            int new_total = total_count + count;
            all_predictions = (int *)realloc(all_predictions, new_total * sizeof(int));
            all_labels = (int *)realloc(all_labels, new_total * sizeof(int));
            memcpy(all_predictions + total_count, predictions, count * sizeof(int));
            memcpy(all_labels + total_count, labels, count * sizeof(int));
            total_count = new_total;

            free(predictions);
            free(labels);
        }

        pcap_close(handle);
        extractor_destroy(extractor);
    }

    double total_end = get_time_in_seconds();
    stats.total_time = stats.feature_extraction_time + stats.inference_time + stats.file_io_time;
    stats.end_to_end_time = total_end - total_start;

    // 打印性能统计
    print_performance_stats(&stats);

    printf("\n过滤统计: 保留 %u 个样本, 过滤掉 %u 个样本\n", combined_filtered.count, combined_filtered.filtered_count);

    // 计算并保存分类报告
    if (total_count > 0) {
        ClassificationReport *report = calculate_classification_report(all_labels, all_predictions, total_count);
        if (report) {
            print_and_save_classification_report(report, output_file);
            free_classification_report(report);
        }

        if (output_file && combined_filtered.count > 0) {
            file_writer_config writer_config = {
                .file_path = output_file, .format = format, .mode = WRITE_MODE_WITH_PREDICTIONS, .append = 0};

            file_writer *writer = writer_create(&writer_config);
            if (writer) {
                feature_vector_list filtered_list = {combined_filtered.vectors, combined_filtered.count,
                                                     combined_filtered.count};

                if (writer_write_features_with_predictions(writer, &filtered_list, all_predictions, total_count) ==
                    WRITER_SUCCESS) {
                    writer_flush(writer);
                    printf("推理结果已保存到: %s\n", output_file);
                }
                writer_destroy(writer);
            }
        }
    }

    free_filtered_features(&combined_filtered);
    free(all_predictions);
    free(all_labels);

    return 0;
}

int online_inference_mode(const char *config_file, const char **pcap_files, int num_pcap_files, const char *interface,
                          int mode, char *output_file, file_format format)
{
    printf("========== 在线推理模式 ==========\n");
    printf("配置文件: %s\n", config_file);

    inference_config inf_config = {.config_file = config_file};
    inference_engine *engine = inference_init(&inf_config);
    if (!engine) {
        fprintf(stderr, "错误: 无法初始化推理引擎\n");
        return -1;
    }

    int result = 0;

    if (interface) {
        result = online_inference_from_interface(engine, interface, output_file, format);
    } else if (num_pcap_files > 0) {
        result = online_inference_from_pcap(engine, pcap_files, num_pcap_files, mode, output_file, format);
    } else {
        fprintf(stderr, "错误: 未指定输入源\n");
        result = -1;
    }

    inference_destroy(engine);
    return result;
}