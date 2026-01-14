#ifndef KDADK_FILE_WRITER_H
#define KDADK_FILE_WRITER_H

#include <stdint.h>
#include "kdadk_feature_extract.h"
#include "kdadk_inference.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 文件格式 */
typedef enum { FILE_FORMAT_CSV = 0, FILE_FORMAT_JSON = 1 } file_format;

/* 写入模式 */
typedef enum {
    WRITE_MODE_FEATURES_ONLY = 0,   /* 只写特征 */
    WRITE_MODE_WITH_PREDICTIONS = 1, /* 写特征+预测结果 */
    WRITE_MODE_FEATURES_WITH_RAWBOW = 2 /* 写特征+rawbow */
} write_mode;

/* 文件写入器配置 */
typedef struct {
    const char *file_path; /* 文件路径 */
    file_format format;    /* 文件格式 */
    write_mode mode;       /* 写入模式 */
    int append;            /* 是否追加模式（0=覆盖，1=追加） */
} file_writer_config;

/* 文件写入器 */
typedef struct file_writer file_writer;

/* 返回值 */
#define WRITER_SUCCESS 0
#define WRITER_ERROR -1

/* ========== 文件写入器接口 ========== */

/**
 * @brief 创建文件写入器
 * @param config 写入器配置
 * @return 文件写入器句柄，失败返回NULL
 */
file_writer *writer_create(const file_writer_config *config);

/**
 * @brief 写入特征向量（不包含预测结果）
 * @param writer 文件写入器句柄
 * @param features 特征向量列表
 * @return WRITER_SUCCESS表示成功，WRITER_ERROR表示失败
 */
int writer_write_features(file_writer *writer, const feature_vector_list *features);

/**
 * @brief 写入特征向量和预测结果
 * @param writer 文件写入器句柄
 * @param features 特征向量列表
 * @param predictions 预测结果数组
 * @param count 数量
 * @return WRITER_SUCCESS表示成功，WRITER_ERROR表示失败
 */
int writer_write_features_with_predictions(file_writer *writer, const feature_vector_list *features,
                                           const int *predictions, uint32_t count);

/**
 * @brief 写入特征向量和rawbow
 * @param writer 文件写入器句柄
 * @param features 特征向量列表
 * @param rawbows rawbow列表
 * @return WRITER_SUCCESS表示成功，WRITER_ERROR表示失败
 */
int writer_write_features_with_rawbow(file_writer *writer, const feature_vector_list *features,
                                      const rawbow_list *rawbows);
/**
 * @brief 刷新缓冲区
 * @param writer 文件写入器句柄
 * @return WRITER_SUCCESS表示成功，WRITER_ERROR表示失败
 */
int writer_flush(file_writer *writer);

/**
 * @brief 销毁文件写入器
 * @param writer 文件写入器句柄
 */
void writer_destroy(file_writer *writer);

#ifdef __cplusplus
}
#endif

#endif /* KDADK_FILE_WRITER_H */