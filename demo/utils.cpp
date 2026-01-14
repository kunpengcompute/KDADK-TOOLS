#include "utils.h"

double get_time_in_seconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 计算准确率
double calculate_accuracy(const int *y_true, const int *y_pred, int count)
{
    int correct = 0;
    for (int i = 0; i < count; i++) {
        if (y_true[i] == y_pred[i]) {
            correct++;
        }
    }
    return count > 0 ? (double)correct / count : 0.0;
}

// 获取唯一类别
void get_unique_classes(const int *y_true, const int *y_pred, int count, int **classes, int *num_classes)
{
    int *temp_classes = (int *)malloc(count * 2 * sizeof(int));
    int temp_count = 0;

    // 收集所有类别
    for (int i = 0; i < count; i++) {
        int found = 0;
        for (int j = 0; j < temp_count; j++) {
            if (temp_classes[j] == y_true[i]) {
                found = 1;
                break;
            }
        }
        if (!found) {
            temp_classes[temp_count++] = y_true[i];
        }

        found = 0;
        for (int j = 0; j < temp_count; j++) {
            if (temp_classes[j] == y_pred[i]) {
                found = 1;
                break;
            }
        }
        if (!found) {
            temp_classes[temp_count++] = y_pred[i];
        }
    }

    // 排序
    for (int i = 0; i < temp_count - 1; i++) {
        for (int j = i + 1; j < temp_count; j++) {
            if (temp_classes[i] > temp_classes[j]) {
                int tmp = temp_classes[i];
                temp_classes[i] = temp_classes[j];
                temp_classes[j] = tmp;
            }
        }
    }

    *classes = temp_classes;
    *num_classes = temp_count;
}

// 计算分类指标
void calculate_class_metrics(const int *classes, int num_classes, const int *y_true, const int *y_pred, int count,
                             ClassificationReport *report)
{
    report->classes = (int *)malloc(num_classes * sizeof(int));
    report->precision = (double *)calloc(num_classes, sizeof(double));
    report->recall = (double *)calloc(num_classes, sizeof(double));
    report->f1_score = (double *)calloc(num_classes, sizeof(double));
    report->support = (int *)calloc(num_classes, sizeof(int));
    report->num_classes = num_classes;

    memcpy(report->classes, classes, num_classes * sizeof(int));

    for (int c_idx = 0; c_idx < num_classes; c_idx++) {
        int c = classes[c_idx];
        int tp = 0, fp = 0, fn = 0;

        for (int i = 0; i < count; i++) {
            if (y_true[i] == c && y_pred[i] == c) {
                tp++;
            } else if (y_pred[i] == c && y_true[i] != c) {
                fp++;
            } else if (y_true[i] == c && y_pred[i] != c) {
                fn++;
            }

            if (y_true[i] == c) {
                report->support[c_idx]++;
            }
        }

        // 计算精确率
        if (tp + fp == 0) {
            report->precision[c_idx] = 0.0;
        } else {
            report->precision[c_idx] = (double)tp / (tp + fp);
        }

        // 计算召回率
        if (tp + fn == 0) {
            report->recall[c_idx] = 0.0;
        } else {
            report->recall[c_idx] = (double)tp / (tp + fn);
        }

        // 计算F1分数
        if (report->precision[c_idx] + report->recall[c_idx] == 0) {
            report->f1_score[c_idx] = 0.0;
        } else {
            report->f1_score[c_idx] = 2 * (report->precision[c_idx] * report->recall[c_idx]) /
                                      (report->precision[c_idx] + report->recall[c_idx]);
        }
    }
}

// 计算平均指标
void calculate_average_metrics(const ClassificationReport *report)
{
    ClassificationReport *rep = (ClassificationReport *)report;
    int total_samples = 0;

    rep->macro_avg_precision = 0.0;
    rep->macro_avg_recall = 0.0;
    rep->macro_avg_f1 = 0.0;
    rep->weighted_avg_precision = 0.0;
    rep->weighted_avg_recall = 0.0;
    rep->weighted_avg_f1 = 0.0;

    // 宏平均
    for (int i = 0; i < report->num_classes; i++) {
        rep->macro_avg_precision += report->precision[i];
        rep->macro_avg_recall += report->recall[i];
        rep->macro_avg_f1 += report->f1_score[i];
    }

    if (report->num_classes > 0) {
        rep->macro_avg_precision /= report->num_classes;
        rep->macro_avg_recall /= report->num_classes;
        rep->macro_avg_f1 /= report->num_classes;
    }

    // 加权平均
    for (int i = 0; i < report->num_classes; i++) {
        rep->weighted_avg_precision += report->precision[i] * report->support[i];
        rep->weighted_avg_recall += report->recall[i] * report->support[i];
        rep->weighted_avg_f1 += report->f1_score[i] * report->support[i];
        total_samples += report->support[i];
    }

    if (total_samples > 0) {
        rep->weighted_avg_precision /= total_samples;
        rep->weighted_avg_recall /= total_samples;
        rep->weighted_avg_f1 /= total_samples;
    }
}

// 生成分类报告
ClassificationReport *calculate_classification_report(const int *y_true, const int *y_pred, int count)
{
    if (count == 0) {
        fprintf(stderr, "错误: 无法计算分类报告，样本数为0\n");
        return NULL;
    }

    ClassificationReport *report = (ClassificationReport *)calloc(1, sizeof(ClassificationReport));

    int *classes = NULL;
    int num_classes = 0;
    get_unique_classes(y_true, y_pred, count, &classes, &num_classes);

    if (num_classes == 0) {
        fprintf(stderr, "错误: 无法计算分类报告，未发现任何类别\n");
        free(report);
        free(classes);
        return NULL;
    }

    calculate_class_metrics(classes, num_classes, y_true, y_pred, count, report);
    report->accuracy = calculate_accuracy(y_true, y_pred, count);
    calculate_average_metrics(report);

    free(classes);
    return report;
}

// 释放分类报告
void free_classification_report(ClassificationReport *report)
{
    if (report) {
        free(report->classes);
        free(report->precision);
        free(report->recall);
        free(report->f1_score);
        free(report->support);
        free(report);
    }
}

static void print_classification_report_to_stream(FILE *fp, const ClassificationReport *report)
{
    if (!fp || !report) {
        return;
    }

    int total_support = 0;
    for (int i = 0; i < report->num_classes; i++) {
        total_support += report->support[i];
    }

    fprintf(fp, "%13s %6s %8s %10s %9s\n\n", "", "precision", "recall", "f1-score", "support");

    for (int i = 0; i < report->num_classes; i++) {
        fprintf(fp, "%12d %9.4f %9.4f %9.4f %9d\n", report->classes[i], report->precision[i], report->recall[i],
                report->f1_score[i], report->support[i]);
    }

    fprintf(fp, "\n");

    fprintf(fp, "%12s %9s %9s %9.4f %9d\n", "accuracy", "", "", report->accuracy, total_support);

    fprintf(fp, "%12s %9.4f %9.4f %9.4f %9d\n", "macro avg", report->macro_avg_precision, report->macro_avg_recall,
            report->macro_avg_f1, total_support);

    fprintf(fp, "%12s %9.4f %9.4f %9.4f %9d\n", "weighted avg", report->weighted_avg_precision,
            report->weighted_avg_recall, report->weighted_avg_f1, total_support);
}

int print_classification_report(const ClassificationReport *report)
{
    if (!report) {
        fprintf(stderr, "错误: 分类报告为空\n");
        return -1;
    }

    printf("\n");
    print_classification_report_to_stream(stdout, report);
    printf("\n");

    return 0;
}

int save_classification_report(const ClassificationReport *report, char *output_file)
{
    if (!report) {
        fprintf(stderr, "错误: 分类报告为空\n");
        return -1;
    }

    // 生成报告文件名
    char report_file[1024];
    char *last_slash = strrchr(output_file, '/');
    if (last_slash) {
        int dir_len = last_slash - output_file + 1;
        strncpy(report_file, output_file, dir_len);
        report_file[dir_len] = '\0';
        strcat(report_file, "classification_report.txt");
    } else {
        strcpy(report_file, "classification_report.txt");
    }

    FILE *fp = fopen(report_file, "w");
    if (!fp) {
        fprintf(stderr, "错误: 无法创建分类报告文件 %s\n", report_file);
        return -1;
    }

    print_classification_report_to_stream(fp, report);

    fclose(fp);
    printf("分类报告已保存到: %s\n", report_file);
    return 0;
}

int print_and_save_classification_report(const ClassificationReport *report, char *output_file)
{
    if (!report) {
        fprintf(stderr, "错误: 分类报告为空\n");
        return -1;
    }

    printf("\n======================= Classification Report =======================\n");
    if (print_classification_report(report) != 0) {
        return -1;
    }

    if (output_file) {
        if (save_classification_report(report, output_file) != 0) {
            return -1;
        }
    }

    return 0;
}

// 打印性能统计
void print_performance_stats(const PerformanceStats *stats)
{
    printf("\n======================= 性能统计 =======================\n");
    printf("特征提取耗时: %10.4f 秒\n", stats->feature_extraction_time);
    printf("推理耗时:     %10.4f 秒\n", stats->inference_time);
    printf("文件IO耗时:   %10.4f 秒\n", stats->file_io_time);
    printf("总耗时:       %10.4f 秒(特征提取 + 推理 + 文件IO)\n", stats->total_time);
    printf("端到端总耗时: %10.4f 秒(包含中间内存分配等开销)\n", stats->end_to_end_time);
    printf("总数据量:     %10.4f MB\n", stats->total_bytes / (1024.0 * 1024.0));

    if (stats->feature_extraction_time > 0) {
        double mbps = (stats->total_bytes / (1024.0 * 1024.0)) / stats->feature_extraction_time;
        double gbps = (stats->total_bytes * 8.0) / (1024.0 * 1024.0 * 1024.0) / stats->feature_extraction_time;
        printf("特征提取吞吐量: %10.4f MB/s (%10.4f Gbps)\n", mbps, gbps);
    }

    if (stats->inference_time > 0) {
        double mbps = (stats->total_bytes / (1024.0 * 1024.0)) / stats->inference_time;
        double gbps = (stats->total_bytes * 8.0) / (1024.0 * 1024.0 * 1024.0) / stats->inference_time;
        printf("推理吞吐量:     %10.4f MB/s (%10.4f Gbps)\n", mbps, gbps);
    }
    if (stats->total_time > 0) {
        double mbps = (stats->total_bytes / (1024.0 * 1024.0)) / stats->total_time;
        double gbps = (stats->total_bytes * 8.0) / (1024.0 * 1024.0 * 1024.0) / stats->total_time;
        printf("核心处理吞吐量: %10.4f MB/s (%10.4f Gbps)\n", mbps, gbps);
    }
    if (stats->end_to_end_time > 0) {
        double mbps = (stats->total_bytes / (1024.0 * 1024.0)) / stats->end_to_end_time;
        double gbps = (stats->total_bytes * 8.0) / (1024.0 * 1024.0 * 1024.0) / stats->end_to_end_time;
        printf("端到端吞吐量:   %10.4f MB/s (%10.4f Gbps)\n", mbps, gbps);
    }
    printf("======================================================\n\n");
}