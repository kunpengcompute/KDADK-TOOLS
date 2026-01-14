#include "kdadk_type.h"
#include "utils.h"
#include "kdadk_inference.h"

void expand_features_capacity(feature_vector_list *fv);
// 在线推理主函数
int online_inference_mode(const char *config_file, const char **pcap_files, int num_pcap_files, const char *interface,
                          int mode, char *output_file);