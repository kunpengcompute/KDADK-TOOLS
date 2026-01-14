
#include "kdadk_type.h"
#include "kdadk_inference.h"
#include "kdadk_file_writer.h"
#include "online_inference.h"

int feature_extraction_mode(const char *pcap_file, const char *output_file, int with_rawbow)
{
    printf("========== 特征提取模式 ==========\n");
    printf("输入文件: %s\n", pcap_file);
    printf("输出文件: %s\n", output_file);
    printf("输出rawbow: %s\n", with_rawbow ? "是" : "否");

    // 初始化特征提取器
    feature_extractor *extractor = extractor_init(DLT_EN10MB);
    if (!extractor) {
        fprintf(stderr, "错误: 无法初始化特征提取器\n");
        return -1;
    }

    // 打开PCAP文件
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_offline(pcap_file, errbuf);
    if (!handle) {
        fprintf(stderr, "%s\n", errbuf);
        extractor_destroy(extractor);
        return -1;
    }

    // 确定输出格式
    file_format format = FILE_FORMAT_CSV;
    if (strstr(output_file, ".json")) {
        format = FILE_FORMAT_JSON;
    }

    // 创建文件写入器
    write_mode mode = with_rawbow ? WRITE_MODE_FEATURES_WITH_RAWBOW : WRITE_MODE_FEATURES_ONLY;
    file_writer_config writer_config = {.file_path = output_file, .format = format, .mode = mode, .append = 0};
    file_writer *writer = writer_create(&writer_config);
    if (!writer) {
        fprintf(stderr, "错误: 无法创建文件写入器\n");
        pcap_close(handle);
        extractor_destroy(extractor);
        return -1;
    }

    // 处理数据包
    struct pcap_pkthdr *header;
    const u_char *packet;
    int packet_count = 0;
    int flow_count = 0;

    feature_vector_list all_features = {0};
    all_features.capacity = 1000;
    all_features.vectors = (feature_vector *)malloc(all_features.capacity * sizeof(feature_vector));
    // 初始化rawbow列表（如果需要）
    rawbow_list all_rawbows = {0};
    if (with_rawbow) {
        all_rawbows.capacity = 1000;
        all_rawbows.rawbows = (rawbow *)malloc(all_rawbows.capacity * sizeof(rawbow));
    }
    while (pcap_next_ex(handle, &header, &packet) >= 0) {
        packet_count++;

        feature_vector features;
        rawbow raw_payload;
        int has_feature = 0;
        int ret;
        if (with_rawbow) {
            ret = process_packet_with_rawbow(extractor, packet, header->caplen, &header->ts, &features, &raw_payload,
                                             &has_feature);
        } else {
            ret = process_packet(extractor, packet, header->caplen, &header->ts, &features, &has_feature);
        }
        if (ret == EXTRACTOR_SUCCESS && has_feature) {
            if (all_features.count >= all_features.capacity) {
                expand_features_capacity(&all_features);
            }
            all_features.vectors[all_features.count++] = features;
            // 扩容rawbow列表（如果需要）
            if (with_rawbow) {
                if (all_rawbows.count >= all_rawbows.capacity) {
                    all_rawbows.capacity *= 2;
                    all_rawbows.rawbows = (rawbow *)realloc(all_rawbows.rawbows, all_rawbows.capacity * sizeof(rawbow));
                }
                all_rawbows.rawbows[all_rawbows.count++] = raw_payload;
            }
            flow_count++;
        }
    }

    // 提取剩余流
    feature_vector_list remaining = {0};
    remaining.capacity = 10000;
    remaining.vectors = (feature_vector *)malloc(remaining.capacity * sizeof(feature_vector));
    rawbow_list remaining_rawbows = {0};
    if (with_rawbow) {
        remaining_rawbows.capacity = 10000;
        remaining_rawbows.rawbows = (rawbow *)malloc(remaining_rawbows.capacity * sizeof(rawbow));
    }
    int ret;
    if (with_rawbow) {
        ret = extractor_finalize_with_rawbow(extractor, &remaining, &remaining_rawbows);
    } else {
        ret = extractor_finalize(extractor, &remaining);
    }
    if (ret == EXTRACTOR_SUCCESS) {
        for (uint32_t i = 0; i < remaining.count; i++) {
            // 扩容特征向量列表
            if (all_features.count >= all_features.capacity) {
                expand_features_capacity(&all_features);
            }
            all_features.vectors[all_features.count++] = remaining.vectors[i];

            // 扩容rawbow列表（如果需要）
            if (with_rawbow && i < remaining_rawbows.count) {
                if (all_rawbows.count >= all_rawbows.capacity) {
                    all_rawbows.capacity *= 2;
                    all_rawbows.rawbows = (rawbow *)realloc(all_rawbows.rawbows, all_rawbows.capacity * sizeof(rawbow));
                }
                all_rawbows.rawbows[all_rawbows.count++] = remaining_rawbows.rawbows[i];
            }

            flow_count++;
        }
        free(remaining.vectors);
        if (with_rawbow) {
            free(remaining_rawbows.rawbows);
        }
    }

    // 写入文件
    if (with_rawbow) {
        if (writer_write_features_with_rawbow(writer, &all_features, &all_rawbows) != WRITER_SUCCESS) {
            fprintf(stderr, "错误: 写入特征和rawbow失败\n");
        }
    } else {
        if (writer_write_features(writer, &all_features) != WRITER_SUCCESS) {
            fprintf(stderr, "错误: 写入特征失败\n");
        }
    }
    writer_flush(writer);

    printf("处理完成:\n");
    printf("  - 总包数: %d\n", packet_count);
    printf("  - 总流数: %d\n", flow_count);
    printf("  - 输出文件: %s\n", output_file);

    // 清理
    free(all_features.vectors);
    if (with_rawbow) {
        free(all_rawbows.rawbows);
    }
    writer_destroy(writer);
    pcap_close(handle);
    extractor_destroy(extractor);

    return 0;
}

/* ========== 命令行参数解析 ========== */
void print_usage(const char *prog_name)
{
    printf("用法: %s [选项]\n", prog_name);
    printf("\n选项:\n");
    printf("  -t <config.yaml>        模型训练模式\n");
    printf("  -e <config.yaml>        模型验证模式\n");
    printf("  -f <file.pcap>          特征提取模式 (需要配合 -o 使用)\n");
    printf("  -r <config.yaml>        在线推理模式 (需要配合 -o 使用)\n");
    printf("  -p <file.pcap>          输入PCAP文件 (可多个)\n");
    printf("  -i <interface>          网口实时抓包\n");
    printf("  -m <mode>               推理模式: 0=单flow, 1=批处理, 2=文件 (默认: 1)\n");
    printf("  -o <output>             输出文件路径 (必需，当使用 -f 或 -r 时)\n");
    printf("  -w                      输出是否带有rawbow (仅用于 -f 特征提取模式)\n");
    printf("  -l                      列出当前机器可用网口\n");
    printf("  -h                      显示帮助信息\n");
    printf("\n示例:\n");
    printf("  %s -f input.pcap -o output.csv\n", prog_name);
    printf("  %s -f input.pcap -o output.csv -w\n", prog_name);
    printf("  %s -r config.yaml -p file1.pcap -p file2.pcap -m 1 -o result.csv\n", prog_name);
    printf("  %s -r config.yaml -i eth0\n", prog_name);
}

void listNetworkDevices()
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs, *device;

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "Error finding devices: %s\n", errbuf);
        return;
    }

    std::cout << "Available network devices:" << std::endl;
    std::cout << "========================================" << std::endl;

    int i = 0;
    for (device = alldevs; device != nullptr; device = device->next) {
        // 过滤掉特殊设备（可选）
        std::string name = device->name;
        if (name.find("usbmon") != std::string::npos || name.find("bluetooth") != std::string::npos ||
            name.find("nflog") != std::string::npos || name.find("nfqueue") != std::string::npos ||
            name.find("any") != std::string::npos) {
            continue;  // 跳过特殊设备
        }

        std::cout << "[" << i++ << "] " << device->name;
        if (device->description) {
            std::cout << " (" << device->description << ")";
        }
        std::cout << std::endl;

        // 显示IP地址
        for (pcap_addr_t *addr = device->addresses; addr != nullptr; addr = addr->next) {
            if (addr->addr && addr->addr->sa_family == AF_INET) {
                struct sockaddr_in *addr_in = (struct sockaddr_in *)addr->addr;
                std::cout << "    IP: " << inet_ntoa(addr_in->sin_addr) << std::endl;
            }
        }
    }

    std::cout << "========================================" << std::endl;
    pcap_freealldevs(alldevs);
}

std::string get_executable_dir()
{
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);

    if (len == -1) {
        perror("readlink failed");
        return ".";
    }

    path[len] = '\0';

    // 手动查找最后一个斜杠（避免 dirname 的问题）
    char *last_slash = strrchr(path, '/');
    if (last_slash != nullptr) {
        *last_slash = '\0';
        return std::string(path);
    }

    return ".";
}

std::string get_script_path(const char *script_name)
{
    if (script_name == nullptr) {  // 额外的安全检查
        return "";
    }

    std::string exe_dir = get_executable_dir();
    return exe_dir + "/../../src/py/" + script_name;
}

// 延迟初始化
const std::string &get_training_script()
{
    static const std::string script = get_script_path("training.py");
    return script;
}

const std::string &get_evaluation_script()
{
    static const std::string script = get_script_path("evaluation.py");
    return script;
}
bool check_file(const std::string &path)
{
    if (access(path.c_str(), F_OK) != 0) {
        fprintf(stderr, "Error: File '%s' not found.\n", path.c_str());
        return false;
    }
    return true;
}

int run_python(const std::string &script, const char *config)
{
    if (!check_file(script) || !check_file(config)) {
        return -1;
    }
    std::string cmd = "python3 " + script + " " + config;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        fprintf(stderr, "Error: Script execution failed.\n");
        return -1;
    }
    return 0;
}

int parse_args(int argc, char *argv[], CommandLineArgs *args)
{
    memset(args, 0, sizeof(CommandLineArgs));
    args->mode = 1;  // 默认批处理模式
    args->operation = -1;
    args->with_rawbow = 0;  // 默认不输出raw payload
    int opt;
    int pcap_capacity = 10;
    args->pcap_files = (char **)malloc(pcap_capacity * sizeof(char *));

    while ((opt = getopt(argc, argv, "t:e:f:r:p:i:m:o:wlh")) != -1) {
        switch (opt) {
            case 't':
                args->operation = 0;
                args->config_file = optarg;
                break;
            case 'e':
                args->operation = 1;
                args->config_file = optarg;
                break;
            case 'f':
                args->operation = 2;
                if (args->num_pcap_files >= pcap_capacity) {
                    pcap_capacity *= 2;
                    args->pcap_files = (char **)realloc(args->pcap_files, pcap_capacity * sizeof(char *));
                }
                args->pcap_files[args->num_pcap_files++] = optarg;
                break;
            case 'r':
                args->operation = 3;
                args->config_file = optarg;
                break;
            case 'p':
                if (args->num_pcap_files >= pcap_capacity) {
                    pcap_capacity *= 2;
                    args->pcap_files = (char **)realloc(args->pcap_files, pcap_capacity * sizeof(char *));
                }
                args->pcap_files[args->num_pcap_files++] = optarg;
                break;
            case 'i':
                args->interface = optarg;
                break;
            case 'm':
                args->mode = atoi(optarg);
                if (args->mode < 0 || args->mode > 2) {
                    fprintf(stderr, "错误: 无效的推理模式 %d\n", args->mode);
                    return -1;
                }
                break;
            case 'o':
                args->output_file = optarg;
                break;
            case 'w':
                args->with_rawbow = 1;  // 启用raw payload输出
                break;
            case 'l':
                listNetworkDevices();
                exit(0);
            case 'h':
                print_usage(argv[0]);
                exit(0);
            default:
                print_usage(argv[0]);
                return -1;
        }
    }

    // ========== 参数验证 ==========

    // 1. 必须指定操作模式
    if (args->operation == -1) {
        fprintf(stderr, "错误: 必须指定操作模式 (-t/-e/-f/-r)\n");
        print_usage(argv[0]);
        return -1;
    }

    // 2. 特征提取模式 (-f) 的验证
    if (args->operation == 2) {
        // 必须有输入PCAP文件
        if (args->num_pcap_files == 0) {
            fprintf(stderr, "错误: 特征提取模式需要指定输入PCAP文件 (-f <file.pcap>)\n");
            print_usage(argv[0]);
            return -1;
        }

        // 必须有输出文件
        if (!args->output_file) {
            fprintf(stderr, "错误: 特征提取模式需要指定输出文件 (-o <output>)\n");
            fprintf(stderr, "提示: 使用 -o 参数指定输出文件路径\n");
            fprintf(stderr, "示例: %s -f input.pcap -o output.csv\n", (argc > 0) ? argv[0] : "kdadk_demo");
            return -1;
        }
    }

    // 3. 在线推理模式 (-r) 的验证
    if (args->operation == 3) {
        // 必须有输入源（PCAP文件或网口）
        if (args->num_pcap_files == 0 && !args->interface) {
            fprintf(stderr, "错误: 在线推理模式需要指定输入源 (-p <file.pcap> 或 -i <interface>)\n");
            print_usage(argv[0]);
            return -1;
        }

        // 如果是PCAP文件输入，必须有输出文件
        if (args->num_pcap_files > 0 && !args->output_file) {
            fprintf(stderr, "错误: 在线推理模式（PCAP文件输入）需要指定输出文件 (-o <output>)\n");
            fprintf(stderr, "提示: 使用 -o 参数指定输出文件路径\n");
            fprintf(stderr, "示例: %s -r config.yaml -p file1.pcap -p file2.pcap -o result.csv\n",
                    (argc > 0) ? argv[0] : "kdadk_demo");
            return -1;
        }

        // 如果是网口实时抓包，输出文件是可选的
        if (args->interface && !args->output_file) {
            printf("提示: 网口实时抓包模式未指定输出文件，推理结果将仅打印到控制台\n");
        }

        // -w 参数只能用于特征提取模式
        if (args->with_rawbow) {
            fprintf(stderr, "警告: -w 参数仅用于特征提取模式 (-f)，在推理模式中将被忽略\n");
            args->with_rawbow = 0;
        }
    }

    // 4. -w 参数只能用于特征提取模式
    if (args->with_rawbow && args->operation != 2) {
        fprintf(stderr, "警告: -w 参数仅用于特征提取模式 (-f)，在其他模式中将被忽略\n");
        args->with_rawbow = 0;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    CommandLineArgs args;

    if (parse_args(argc, argv, &args) != 0) {
        return 1;
    }

    int ret = 0;

    switch (args.operation) {
        case 0:
            printf("========== 模型训练模式 ==========\n");
            return run_python(get_training_script(), args.config_file);

        case 1:
            printf("========== 模型验证模式 ==========\n");
            return run_python(get_evaluation_script(), args.config_file);

        case 2:
            // 特征提取模式
            if (args.num_pcap_files > 0) {
                ret = feature_extraction_mode(args.pcap_files[0], args.output_file, args.with_rawbow);
            }
            break;

        case 3:
            // 在线推理模式
            ret = online_inference_mode(args.config_file, (const char **)args.pcap_files, args.num_pcap_files,
                                        args.interface, args.mode, args.output_file);
            break;

        default:
            fprintf(stderr, "错误: 未知的操作模式\n");
            ret = 1;
    }

    free(args.pcap_files);
    return ret;
}