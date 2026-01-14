#include "online_inference.h"
#include "kdadk_file_writer.h"

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

// 模式0: 单flow模式
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

    // 用于保存所有过滤后的特征
    all_filtered_out->capacity = 1000;
    all_filtered_out->vectors = (feature_vector *)malloc(all_filtered_out->capacity * sizeof(feature_vector));
    all_filtered_out->count = 0;
    all_filtered_out->filtered_count = 0;

    double extract_time = 0, infer_time = 0;

    while (pcap_next_ex(handle, &header, &packet) >= 0) {
        feature_vector features;
        int has_feature = 0;

        double t1 = get_time_in_seconds();
        if (process_packet(extractor, packet, header->caplen, &header->ts, &features, &has_feature) ==
            EXTRACTOR_SUCCESS) {
            double t2 = get_time_in_seconds();
            extract_time += (t2 - t1);

            if (has_feature) {
                // 单flow推理
                feature_vector_list fv_list = {&features, 1, 1};

                // 过滤特征
                filtered_features filtered = {0};
                if (filter_features(engine, &fv_list, &filtered) == 0 && filtered.count > 0) {
                    // 保存过滤后的特征
                    if (all_filtered_out->count >= all_filtered_out->capacity) {
                        all_filtered_out->capacity *= 2;
                        all_filtered_out->vectors = (feature_vector *)realloc(
                            all_filtered_out->vectors, all_filtered_out->capacity * sizeof(feature_vector));
                    }
                    all_filtered_out->vectors[all_filtered_out->count++] = filtered.vectors[0];
                    all_filtered_out->filtered_count += filtered.filtered_count;

                    // 推理
                    inference_result result;
                    result.predictions = (int *)malloc(sizeof(int));
                    result.count = 1;

                    double t3 = get_time_in_seconds();
                    if (inference_predict(engine, &fv_list, &result) == INFERENCE_SUCCESS) {
                        double t4 = get_time_in_seconds();
                        infer_time += (t4 - t3);

                        // 保存结果
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
        stats->total_bytes += header->caplen;
    }

    // 处理剩余流
    feature_vector_list remaining = {0};
    remaining.capacity = 20000;
    remaining.vectors = (feature_vector *)malloc(remaining.capacity * sizeof(feature_vector));

    double t1 = get_time_in_seconds();
    if (extractor_finalize(extractor, &remaining) == EXTRACTOR_SUCCESS) {
        double t2 = get_time_in_seconds();
        extract_time += (t2 - t1);

        if (remaining.count > 0) {
            // 过滤特征
            filtered_features filtered = {0};
            if (filter_features(engine, &remaining, &filtered) == 0 && filtered.count > 0) {
                // 保存过滤后的特征
                for (uint32_t i = 0; i < filtered.count; i++) {
                    if (all_filtered_out->count >= all_filtered_out->capacity) {
                        all_filtered_out->capacity *= 2;
                        all_filtered_out->vectors = (feature_vector *)realloc(
                            all_filtered_out->vectors, all_filtered_out->capacity * sizeof(feature_vector));
                    }
                    all_filtered_out->vectors[all_filtered_out->count++] = filtered.vectors[i];
                }
                all_filtered_out->filtered_count += filtered.filtered_count;

                // 推理
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

    stats->feature_extraction_time += extract_time;
    stats->inference_time += infer_time;

    return 0;
}

// 模式1: 批处理模式
int inference_mode_batch(feature_extractor *extractor, inference_engine *engine, pcap_t *handle, int ground_truth_label,
                         filtered_features *all_filtered_out, int **all_predictions, int **all_labels, int *total_count,
                         PerformanceStats *stats)
{
    struct pcap_pkthdr *header;
    const u_char *packet;

    feature_vector_list all_features = {0};
    all_features.capacity = 1000;
    all_features.vectors = (feature_vector *)malloc(all_features.capacity * sizeof(feature_vector));

    double extract_time = 0;

    // 提取所有特征
    while (pcap_next_ex(handle, &header, &packet) >= 0) {
        feature_vector features;
        int has_feature = 0;

        double t1 = get_time_in_seconds();
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
        stats->total_bytes += header->caplen;
    }

    // 提取剩余流
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

    stats->feature_extraction_time += extract_time;

    // 过滤特征
    filtered_features filtered = {0};
    if (filter_features(engine, &all_features, &filtered) == 0 && filtered.count > 0) {
        // 保存过滤后的特征
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

// 模式2: 文件模式
int inference_mode_file(feature_extractor *extractor, inference_engine *engine, pcap_t *handle, int ground_truth_label,
                        filtered_features *all_filtered_out, int **all_predictions, int **all_labels, int *total_count,
                        PerformanceStats *stats, const char *output_file)
{
    struct pcap_pkthdr *header;
    const u_char *packet;

    // 临时文件
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

    double extract_time = 0, io_time = 0;

    // 提取特征并写入文件
    while (pcap_next_ex(handle, &header, &packet) >= 0) {
        feature_vector features;
        int has_feature = 0;

        double t1 = get_time_in_seconds();
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
        stats->total_bytes += header->caplen;
    }

    // 提取剩余流
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

    // 释放原始特征（模拟真实场景中内存被释放）
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
        // 保存过滤后的特征
        *all_filtered_out = filtered;

        // 推理
        inference_result result;
        result.predictions = (int *)malloc(filtered.count * sizeof(int));
        result.count = filtered.count;

        double t5 = get_time_in_seconds();
        if (inference_predict(engine, &loaded_features, &result) == INFERENCE_SUCCESS) {
            double t6 = get_time_in_seconds();
            stats->inference_time += (t6 - t5);

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

    // 删除临时文件
    remove(temp_file);

    return 0;
}

// 在线推理主函数
int online_inference_mode(const char *config_file, const char **pcap_files, int num_pcap_files, const char *interface,
                          int mode, char *output_file)
{
    printf("========== 在线推理模式 ==========\n");
    printf("配置文件: %s\n", config_file);
    printf("推理模式: %d\n", mode);

    // 初始化推理引擎
    inference_config inf_config = {.config_file = config_file};
    inference_engine *engine = inference_init(&inf_config);
    if (!engine) {
        fprintf(stderr, "错误: 无法初始化推理引擎\n");
        return -1;
    }

    PerformanceStats stats = {0};
    double total_start = get_time_in_seconds();

    int *all_predictions = NULL;
    int *all_labels = NULL;
    int total_count = 0;

    // 用于保存所有过滤后的特征
    filtered_features combined_filtered = {0};
    combined_filtered.capacity = 10000;
    combined_filtered.vectors = (feature_vector *)malloc(combined_filtered.capacity * sizeof(feature_vector));
    combined_filtered.count = 0;
    combined_filtered.filtered_count = 0;

    // 处理网口实时抓包
    if (interface) {
        printf("从网口抓包: %s\n", interface);
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_t *handle = pcap_open_live(interface, 65535, 1, 1000, errbuf);
        if (!handle) {
            fprintf(stderr, "错误: 无法打开网口: %s\n", errbuf);
            inference_destroy(engine);
            free(combined_filtered.vectors);
            return -1;
        }

        feature_extractor *extractor = extractor_init(DLT_EN10MB);

        // 实时抓包不计算准确率，只进行推理
        struct pcap_pkthdr *header;
        const u_char *packet;
        int flow_count = 0;

        printf("开始实时抓包 (Ctrl+C 退出)...\n");
        while (pcap_next_ex(handle, &header, &packet) >= 0) {
            feature_vector features;
            int has_feature = 0;

            if (process_packet(extractor, packet, header->caplen, &header->ts, &features, &has_feature) ==
                EXTRACTOR_SUCCESS) {
                if (has_feature) {
                    feature_vector_list fv_list = {&features, 1, 1};

                    // 过滤特征
                    filtered_features filtered = {0};
                    if (filter_features(engine, &fv_list, &filtered) == 0 && filtered.count > 0) {
                        // 推理
                        inference_result result;
                        result.predictions = (int *)malloc(sizeof(int));
                        result.count = 1;

                        if (inference_predict(engine, &fv_list, &result) == INFERENCE_SUCCESS) {
                            printf("Flow %d: 预测类别 = %d\n", flow_count++, result.predictions[0]);
                        }
                        free(result.predictions);
                        free_filtered_features(&filtered);
                    }
                }
            }
        }

        // 处理剩余流
        feature_vector_list remaining = {0};
        remaining.capacity = 10000;
        remaining.vectors = (feature_vector *)malloc(remaining.capacity * sizeof(feature_vector));

        if (extractor_finalize(extractor, &remaining) == EXTRACTOR_SUCCESS) {
            if (remaining.count > 0) {
                // 过滤特征
                filtered_features filtered = {0};
                if (filter_features(engine, &remaining, &filtered) == 0 && filtered.count > 0) {
                    // 推理
                    inference_result result;
                    result.predictions = (int *)malloc(filtered.count * sizeof(int));
                    result.count = filtered.count;

                    if (inference_predict(engine, &remaining, &result) == INFERENCE_SUCCESS) {
                        for (uint32_t i = 0; i < result.count; i++) {
                            printf("Flow %d: 预测类别 = %d\n", flow_count++, result.predictions[i]);
                        }
                    }
                    free(result.predictions);
                    free_filtered_features(&filtered);
                }
            }
            free(remaining.vectors);
        }

        pcap_close(handle);
        extractor_destroy(extractor);
        inference_destroy(engine);
        free(combined_filtered.vectors);
        return 0;
    }

    // 处理PCAP文件
    for (int i = 0; i < num_pcap_files; i++) {
        printf("\n处理文件 %d/%d: %s (Label=%d)\n", i + 1, num_pcap_files, pcap_files[i], i);

        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_t *handle = pcap_open_offline(pcap_files[i], errbuf);
        if (!handle) {
            fprintf(stderr, "错误: 无法打开PCAP文件: %s\n", errbuf);
            continue;
        }

        feature_extractor *extractor = extractor_init(DLT_EN10MB);

        int *predictions = NULL;
        int *labels = NULL;
        int count = 0;
        filtered_features file_filtered = {0};

        // 根据模式选择处理方式
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
            printf("\n准确率: %.6f\n", report->accuracy);
            print_and_save_classification_report(report, output_file);
            free_classification_report(report);
        }

        // 保存推理结果（原始特征 + 预测结果）
        if (output_file && combined_filtered.count > 0) {
            file_format format = FILE_FORMAT_CSV;
            if (strstr(output_file, ".json")) {
                format = FILE_FORMAT_JSON;
            }

            file_writer_config writer_config = {
                .file_path = output_file, .format = format, .mode = WRITE_MODE_WITH_PREDICTIONS, .append = 0};
            file_writer *writer = writer_create(&writer_config);
            if (writer) {
                feature_vector_list filtered_list = {combined_filtered.vectors, combined_filtered.count,
                                                     combined_filtered.count};
                writer_write_features_with_predictions(writer, &filtered_list, all_predictions, total_count);
                writer_flush(writer);
                writer_destroy(writer);
                printf("推理结果已保存到: %s\n", output_file);
            }
        }
    }

    free_filtered_features(&combined_filtered);
    free(all_predictions);
    free(all_labels);
    inference_destroy(engine);

    return 0;
}