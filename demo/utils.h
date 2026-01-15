/*
Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 */
#include "kdadk_type.h"

double get_time_in_seconds();
double calculate_accuracy(const int *y_true, const int *y_pred, int count);
void get_unique_classes(const int *y_true, const int *y_pred, int count, int **classes, int *num_classes);
void calculate_class_metrics(const int *classes, int num_classes, const int *y_true, const int *y_pred, int count,
                             ClassificationReport *report);
void calculate_average_metrics(const ClassificationReport *report);
ClassificationReport *calculate_classification_report(const int *y_true, const int *y_pred, int count);
void free_classification_report(ClassificationReport *report);
static void print_classification_report_to_stream(FILE *fp, const ClassificationReport *report);
int print_classification_report(const ClassificationReport *report);
int save_classification_report(const ClassificationReport *report, char *output_file);
int print_and_save_classification_report(const ClassificationReport *report, char *output_file);
void print_performance_stats(const PerformanceStats *stats);