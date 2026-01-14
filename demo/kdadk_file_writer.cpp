#include "kdadk_file_writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

using namespace std;

/* 直方图数量常量 */
#define HISTOGRAM_NUMS 16
#define RAW_BOW_HISTOGRAM_NUMS 32

/* 文件写入器结构 */
struct file_writer {
    FILE *fp;
    file_format format;
    write_mode mode;
    int header_written;
    string file_path;

    // JSON相关
    bool first_record;
};

/* ========== CSV写入相关函数 ========== */

static int write_csv_header(FILE *fp, write_mode mode)
{
    if (!fp) {
        return WRITER_ERROR;
    }

    // 五元组
    const char *tupleHeader[5] = {"src_ip", "dst_ip", "src_port", "dst_port", "protocol"};
    const char *direct[2] = {"send", "receive"};
    const char *type[3] = {"hdr", "pld", "interval"};

    // 写入flow_id
    fprintf(fp, "flow_id,");

    // 写入五元组
    for (int i = 0; i < 5; i++) {
        fprintf(fp, "%s,", tupleHeader[i]);
    }

    // 写入统计特征
    for (int i = 0; i < 2; i++) {
        fprintf(fp, "%s_byte_nums,", direct[i]);
        fprintf(fp, "%s_packet_nums,", direct[i]);
        for (int j = 0; j < 3; j++) {
            fprintf(fp, "%s_%s_min,", direct[i], type[j]);
            fprintf(fp, "%s_%s_max,", direct[i], type[j]);
            fprintf(fp, "%s_%s_mean,", direct[i], type[j]);
            fprintf(fp, "%s_%s_var,", direct[i], type[j]);

            for (int k = 0; k < HISTOGRAM_NUMS; k++) {
                fprintf(fp, "%s_%s_hist_rel_%d,", direct[i], type[j], k);
            }

            for (int k = 0; k < HISTOGRAM_NUMS; k++) {
                fprintf(fp, "%s_%s_hist_abs_%d,", direct[i], type[j], k);
            }

            for (int k = 0; k < HISTOGRAM_NUMS; k++) {
                fprintf(fp, "%s_%s_hist_nor_%d,", direct[i], type[j], k);
            }
        }
    }

    // 写入raw_bow特征
    for (int i = 0; i < RAW_BOW_HISTOGRAM_NUMS - 1; i++) {
        fprintf(fp, "raw_bow_crc_hist_%d,", i);
    }
    fprintf(fp, "raw_bow_crc_hist_%d", RAW_BOW_HISTOGRAM_NUMS - 1);
    // 如果需要写入预测结果
    if (mode == WRITE_MODE_WITH_PREDICTIONS) {
        fprintf(fp, ",prediction");
    }

    fprintf(fp, "\n");
    return WRITER_SUCCESS;
}

static int write_csv_header_with_rawbow(FILE *fp)
{
    if (!fp) {
        return WRITER_ERROR;
    }

    // 五元组
    const char *tupleHeader[5] = {"src_ip", "dst_ip", "src_port", "dst_port", "protocol"};
    const char *direct[2] = {"send", "receive"};
    const char *type[3] = {"hdr", "pld", "interval"};

    // 写入flow_id
    fprintf(fp, "flow_id,");

    // 写入五元组
    for (int i = 0; i < 5; i++) {
        fprintf(fp, "%s,", tupleHeader[i]);
    }

    // 写入统计特征
    for (int i = 0; i < 2; i++) {
        fprintf(fp, "%s_byte_nums,", direct[i]);
        fprintf(fp, "%s_packet_nums,", direct[i]);
        for (int j = 0; j < 3; j++) {
            fprintf(fp, "%s_%s_min,", direct[i], type[j]);
            fprintf(fp, "%s_%s_max,", direct[i], type[j]);
            fprintf(fp, "%s_%s_mean,", direct[i], type[j]);
            fprintf(fp, "%s_%s_var,", direct[i], type[j]);

            for (int k = 0; k < HISTOGRAM_NUMS; k++) {
                fprintf(fp, "%s_%s_hist_rel_%d,", direct[i], type[j], k);
            }

            for (int k = 0; k < HISTOGRAM_NUMS; k++) {
                fprintf(fp, "%s_%s_hist_abs_%d,", direct[i], type[j], k);
            }

            for (int k = 0; k < HISTOGRAM_NUMS; k++) {
                fprintf(fp, "%s_%s_hist_nor_%d,", direct[i], type[j], k);
            }
        }
    }

    // 写入raw_bow特征（CRC直方图）
    for (int i = 0; i < RAW_BOW_HISTOGRAM_NUMS; i++) {
        fprintf(fp, "raw_bow_crc_hist_%d,", i);
    }

    // 写入rawbow列
    fprintf(fp, "rawbow\n");

    return WRITER_SUCCESS;
}

static int write_csv_feature_row(FILE *fp, const feature_vector *fv, int prediction, write_mode mode)
{
    if (!fp || !fv) {
        return WRITER_ERROR;
    }

    // 写入所有特征（包括flow_id）
    for (int i = 0; i < FEATURES_NUM - 1; i++) {
        fprintf(fp, "%.6f", fv->features[i]);
        if (i < FEATURES_NUM - 2) {
            fprintf(fp, ",");
        }
    }

    // 如果需要写入预测结果
    if (mode == WRITE_MODE_WITH_PREDICTIONS) {
        fprintf(fp, ",%d", prediction);
    }

    fprintf(fp, "\n");
    return WRITER_SUCCESS;
}

/* ========== JSON写入相关函数 ========== */

static int write_json_header(FILE *fp)
{
    if (!fp) {
        return WRITER_ERROR;
    }
    fprintf(fp, "[\n");
    return WRITER_SUCCESS;
}

static int write_json_footer(FILE *fp)
{
    if (!fp) {
        return WRITER_ERROR;
    }
    fprintf(fp, "\n]\n");
    return WRITER_SUCCESS;
}

static int write_json_feature_row(FILE *fp, const feature_vector *fv, int prediction, write_mode mode, bool is_first)
{
    if (!fp || !fv) {
        return WRITER_ERROR;
    }

    // 如果不是第一条记录，先写逗号
    if (!is_first) {
        fprintf(fp, ",\n");
    }

    fprintf(fp, "  {\n");

    // 写入flow_id
    fprintf(fp, "    \"flow_id\": %.0f,\n", fv->features[0]);

    // 写入五元组
    fprintf(fp, "    \"src_ip\": \"%.0f\",\n", fv->features[1]);
    fprintf(fp, "    \"dst_ip\": \"%.0f\",\n", fv->features[2]);
    fprintf(fp, "    \"src_port\": %.0f,\n", fv->features[3]);
    fprintf(fp, "    \"dst_port\": %.0f,\n", fv->features[4]);
    fprintf(fp, "    \"protocol\": %.0f,\n", fv->features[5]);

    // 写入所有特征作为数组
    fprintf(fp, "    \"features\": [");
    for (int i = 0; i < FEATURES_NUM; i++) {
        fprintf(fp, "%.6f", fv->features[i]);
        if (i < FEATURES_NUM - 1) {
            fprintf(fp, ", ");
        }
    }
    fprintf(fp, "]");

    // 如果需要写入预测结果
    if (mode == WRITE_MODE_WITH_PREDICTIONS) {
        fprintf(fp, ",\n    \"prediction\": %d", prediction);
    }

    fprintf(fp, "\n  }");

    return WRITER_SUCCESS;
}

file_writer *writer_create(const file_writer_config *config)
{
    if (!config || !config->file_path) {
        fprintf(stderr, "Error: Invalid config or file_path is NULL\n");
        return NULL;
    }

    file_writer *writer = new file_writer();
    if (!writer) {
        fprintf(stderr, "Error: Failed to allocate memory for file_writer\n");
        return NULL;
    }

    writer->format = config->format;
    writer->mode = config->mode;
    writer->header_written = 0;
    writer->first_record = true;
    writer->file_path = config->file_path;

    const char *mode_str = config->append ? "a" : "w";
    writer->fp = fopen(config->file_path, mode_str);
    if (!writer->fp) {
        fprintf(stderr, "Error: Failed to open file: %s\n", config->file_path);
        delete writer;
        return NULL;
    }

    if (!config->append) {
        if (config->format == FILE_FORMAT_CSV) {
            if (config->mode == WRITE_MODE_FEATURES_WITH_RAWBOW) {
                write_csv_header_with_rawbow(writer->fp);
            } else {
                write_csv_header(writer->fp, config->mode);
            }
            writer->header_written = 1;
        } else if (config->format == FILE_FORMAT_JSON) {
            write_json_header(writer->fp);
            writer->header_written = 1;
        }
    } else {
        writer->header_written = 1;
        writer->first_record = false;
    }

    const char *mode_name;
    switch (config->mode) {
        case WRITE_MODE_FEATURES_ONLY:
            mode_name = "features_only";
            break;
        case WRITE_MODE_WITH_PREDICTIONS:
            mode_name = "with_predictions";
            break;
        case WRITE_MODE_FEATURES_WITH_RAWBOW:
            mode_name = "features_with_rawbow";
            break;
        default:
            mode_name = "unknown";
            break;
    }

    printf("File writer created: %s (format=%s, mode=%s)\n", config->file_path,
           config->format == FILE_FORMAT_CSV ? "CSV" : "JSON", mode_name);

    return writer;
}

int writer_write_features(file_writer *writer, const feature_vector_list *features)
{
    if (!writer || !writer->fp) {
        fprintf(stderr, "Error: Invalid writer or file not opened\n");
        return WRITER_ERROR;
    }

    if (!features || !features->vectors || features->count == 0) {
        fprintf(stderr, "Error: Invalid features\n");
        return WRITER_ERROR;
    }

    if (writer->mode != WRITE_MODE_FEATURES_ONLY) {
        fprintf(stderr, "Error: Writer mode mismatch (expected FEATURES_ONLY)\n");
        return WRITER_ERROR;
    }

    // 写入每个特征向量
    for (uint32_t i = 0; i < features->count; i++) {
        if (writer->format == FILE_FORMAT_CSV) {
            if (write_csv_feature_row(writer->fp, &features->vectors[i], 0, writer->mode) != WRITER_SUCCESS) {
                return WRITER_ERROR;
            }
        } else if (writer->format == FILE_FORMAT_JSON) {
            if (write_json_feature_row(writer->fp, &features->vectors[i], 0, writer->mode, writer->first_record) !=
                WRITER_SUCCESS) {
                return WRITER_ERROR;
            }
            writer->first_record = false;
        }
    }

    return WRITER_SUCCESS;
}

static int write_csv_feature_row_with_rawbow(FILE *fp, const feature_vector *fv, const rawbow *rb)
{
    if (!fp || !fv || !rb) {
        return WRITER_ERROR;
    }

    for (int i = 0; i < FEATURES_NUM - 1; i++) {
        fprintf(fp, "%.6f,", fv->features[i]);
    }

    fprintf(fp, "\"");

    for (uint32_t i = 0; i < rb->len && i < RAWBOW_MAX_LEN && rb->rawbow[i] != '\0'; i++) {
        char c = rb->rawbow[i];

        if (c == '"') {
            fprintf(fp, "\"\"");
        } else if (c == '\n') {
            fprintf(fp, "\\n");
        } else if (c == '\r') {
            fprintf(fp, "\\r");
        } else if (c == '\\') {
            fprintf(fp, "\\\\");
        } else {
            fputc(c, fp);
        }
    }

    fprintf(fp, "\"\n");

    return WRITER_SUCCESS;
}

static int write_json_feature_row_with_rawbow(FILE *fp, const feature_vector *fv, const rawbow *rb, bool is_first)
{
    if (!fp || !fv || !rb) {
        return WRITER_ERROR;
    }

    if (!is_first) {
        fprintf(fp, ",\n");
    }

    fprintf(fp, "  {\n");

    fprintf(fp, "    \"flow_id\": %.0f,\n", fv->features[0]);

    fprintf(fp, "    \"src_ip\": \"%.0f\",\n", fv->features[1]);
    fprintf(fp, "    \"dst_ip\": \"%.0f\",\n", fv->features[2]);
    fprintf(fp, "    \"src_port\": %.0f,\n", fv->features[3]);
    fprintf(fp, "    \"dst_port\": %.0f,\n", fv->features[4]);
    fprintf(fp, "    \"protocol\": %.0f,\n", fv->features[5]);

    fprintf(fp, "    \"features\": [");
    for (int i = 0; i < FEATURES_NUM; i++) {
        fprintf(fp, "%.6f", fv->features[i]);
        if (i < FEATURES_NUM - 1) {
            fprintf(fp, ", ");
        }
    }
    fprintf(fp, "],\n");

    fprintf(fp, "    \"rawbow\": \"");

    for (uint32_t i = 0; i < rb->len && i < RAWBOW_MAX_LEN && rb->rawbow[i] != '\0'; i++) {
        char c = rb->rawbow[i];

        switch (c) {
            case '"':
                fprintf(fp, "\\\"");
                break;
            case '\\':
                fprintf(fp, "\\\\");
                break;
            case '\b':
                fprintf(fp, "\\b");
                break;
            case '\f':
                fprintf(fp, "\\f");
                break;
            case '\n':
                fprintf(fp, "\\n");
                break;
            case '\r':
                fprintf(fp, "\\r");
                break;
            case '\t':
                fprintf(fp, "\\t");
                break;
            default:
                if ((unsigned char)c < 0x20) {
                    fprintf(fp, "\\u%04x", (unsigned char)c);
                } else {
                    fputc(c, fp);
                }
                break;
        }
    }

    fprintf(fp, "\"\n");
    fprintf(fp, "  }");

    return WRITER_SUCCESS;
}

int writer_write_features_with_rawbow(file_writer *writer, const feature_vector_list *features,
                                      const rawbow_list *rawbows)
{
    if (!writer || !writer->fp) {
        fprintf(stderr, "Error: Invalid writer or file not opened\n");
        return WRITER_ERROR;
    }

    if (!features || !features->vectors || features->count == 0) {
        fprintf(stderr, "Error: Invalid features\n");
        return WRITER_ERROR;
    }

    if (!rawbows || !rawbows->rawbows) {
        fprintf(stderr, "Error: Invalid rawbows\n");
        return WRITER_ERROR;
    }

    // 验证数量一致性
    if (features->count != rawbows->count) {
        fprintf(stderr, "Error: Feature count (%u) != rawbow count (%u)\n", features->count, rawbows->count);
        return WRITER_ERROR;
    }

    if (writer->mode != WRITE_MODE_FEATURES_WITH_RAWBOW) {
        fprintf(stderr, "Error: Writer mode mismatch (expected FEATURES_WITH_RAWBOW)\n");
        return WRITER_ERROR;
    }

    // 如果还没有写入头部，先写入头部
    if (!writer->header_written) {
        if (writer->format == FILE_FORMAT_CSV) {
            if (write_csv_header_with_rawbow(writer->fp) != WRITER_SUCCESS) {
                fprintf(stderr, "Error: Failed to write CSV header\n");
                return WRITER_ERROR;
            }
            writer->header_written = 1;
        } else if (writer->format == FILE_FORMAT_JSON) {
            if (write_json_header(writer->fp) != WRITER_SUCCESS) {
                fprintf(stderr, "Error: Failed to write JSON header\n");
                return WRITER_ERROR;
            }
            writer->header_written = 1;
        }
    }

    // 写入每个特征向量和对应的rawbow
    for (uint32_t i = 0; i < features->count; i++) {
        if (writer->format == FILE_FORMAT_CSV) {
            if (write_csv_feature_row_with_rawbow(writer->fp, &features->vectors[i], &rawbows->rawbows[i]) !=
                WRITER_SUCCESS) {
                fprintf(stderr, "Error: Failed to write CSV row %u\n", i);
                return WRITER_ERROR;
            }
        } else if (writer->format == FILE_FORMAT_JSON) {
            if (write_json_feature_row_with_rawbow(writer->fp, &features->vectors[i], &rawbows->rawbows[i],
                                                   writer->first_record) != WRITER_SUCCESS) {
                fprintf(stderr, "Error: Failed to write JSON row %u\n", i);
                return WRITER_ERROR;
            }
            writer->first_record = false;
        }
    }

    printf("Successfully wrote %u features with rawbow\n", features->count);
    return WRITER_SUCCESS;
}

int writer_write_features_with_predictions(file_writer *writer, const feature_vector_list *features,
                                           const int *predictions, uint32_t count)
{
    if (!writer || !writer->fp) {
        fprintf(stderr, "Error: Invalid writer or file not opened\n");
        return WRITER_ERROR;
    }

    if (!features || !features->vectors || !predictions) {
        fprintf(stderr, "Error: Invalid features or predictions\n");
        return WRITER_ERROR;
    }

    if (features->count != count) {
        fprintf(stderr, "Error: Feature count (%u) != prediction count (%u)\n", features->count, count);
        return WRITER_ERROR;
    }

    if (writer->mode != WRITE_MODE_WITH_PREDICTIONS) {
        fprintf(stderr, "Error: Writer mode mismatch (expected WITH_PREDICTIONS)\n");
        return WRITER_ERROR;
    }

    // 写入每个特征向量和预测结果
    for (uint32_t i = 0; i < count; i++) {
        if (writer->format == FILE_FORMAT_CSV) {
            if (write_csv_feature_row(writer->fp, &features->vectors[i], predictions[i], writer->mode) !=
                WRITER_SUCCESS) {
                return WRITER_ERROR;
            }
        } else if (writer->format == FILE_FORMAT_JSON) {
            if (write_json_feature_row(writer->fp, &features->vectors[i], predictions[i], writer->mode,
                                       writer->first_record) != WRITER_SUCCESS) {
                return WRITER_ERROR;
            }
            writer->first_record = false;
        }
    }

    return WRITER_SUCCESS;
}

int writer_flush(file_writer *writer)
{
    if (!writer || !writer->fp) {
        fprintf(stderr, "Error: Invalid writer or file not opened\n");
        return WRITER_ERROR;
    }

    fflush(writer->fp);
    return WRITER_SUCCESS;
}

void writer_destroy(file_writer *writer)
{
    if (!writer) {
        return;
    }

    if (writer->fp) {
        // 如果是JSON格式，写入结束标记
        if (writer->format == FILE_FORMAT_JSON && writer->header_written) {
            write_json_footer(writer->fp);
        }

        fclose(writer->fp);
    }

    printf("File writer destroyed\n");
    delete writer;
}