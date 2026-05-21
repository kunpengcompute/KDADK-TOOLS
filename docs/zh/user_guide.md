# 用户指南

本用户指南提供了`kdadk_demo`示例的使用方法。同时，也提供了流量采集工具和流量打标签工具的使用方法。

## 示例用法

`kdadk_demo`可以使用`-h`参数查看程序功能。

```bash
/opt/KDADK-TOOLS/demo/build/kdadk_demo -h
```

出现下述回显:

```bash
用法: /opt/KDADK-TOOLS/demo/build/kdadk_demo [选项]

选项:
  -t <config.yaml>        模型训练模式
  -e <config.yaml>        模型验证模式
  -f <file.pcap>          特征提取模式 (需要配合 -c 或 -j 使用)
  -r <config.yaml>        在线推理模式 (需要配合 -c 或 -j 使用)
  -p <file.pcap>          输入PCAP文件 (可多个)
  -i <interface>          网口实时抓包
  -m <mode>               推理模式: 0=单flow, 1=批处理, 2=文件 (默认: 1)
  -c <output.csv>         输出CSV文件路径 (与 -j 互斥)
  -j <output.json>        输出JSON文件路径 (与 -c 互斥)
  -w                      输出带有rawbow (仅用于 -f 特征提取模式)
  -l                      列出当前服务器可用网口
  -h                      显示帮助信息

说明:
  -c 和 -j 选项只能选择其中一个，分别指定CSV或JSON格式的输出文件
  -c 选项的文件必须以 .csv 结尾
  -j 选项的文件必须以 .json 结尾
  -w rawbow输出仅适用于 -f 特征提取模式, rawbow特征列信息仅用于流量打标签进行人工校验

示例:
  # 特征提取 - CSV格式
  /opt/KDADK-TOOLS/demo/build/kdadk_demo -f input.pcap -c output.csv

  # 特征提取 - JSON格式，带rawbow
  /opt/KDADK-TOOLS/demo/build/kdadk_demo -f input.pcap -j output.json -w

  # 在线推理 - 多个PCAP文件，输出CSV
  /opt/KDADK-TOOLS/demo/build/kdadk_demo -r config.yaml -p file1.pcap -p file2.pcap -m 1 -c result.csv

  # 在线推理 - 网口实时抓包，输出JSON
  /opt/KDADK-TOOLS/demo/build/kdadk_demo -r config.yaml -i eth0 -j result.json

  # 列出可用网口
  /opt/KDADK-TOOLS/demo/build/kdadk_demo -l
```

其中核心功能模块有：模型训练(-t)、模型验证(-e)、特征提取(-f)、在线推理(-r)。

### 1. 模型训练 (-t)

- 参数：`-t config.yaml`
- 功能：读取[config配置](#7-config配置)文件，调用模型训练脚本（Python），执行模型训练任务。
- config参数：`training_data_paths、model_path_pkl、scaler_path_pkl、model_path_onnx、scaler_path_json`。

### 2. 模型验证 (-e)

- 参数：`-e config.yaml`
- 功能：读取[config配置](#7-config配置)文件，调用模型验证脚本（Python），执行模型验证。
- config参数：`evaluation_data_paths、model_path_pkl、scaler_path_pkl、output_dir、classification_report_file、predictions_detail_file`。

### 3. 特征提取 (-f)

- 参数：`-f xxx.pcap -c output.csv`或`-f xxx.pcap -j output.json`
- 功能：输入pcap离线网络流量，进行协议解析及特征提取。
- 输出：将特征提取结果保存在文件中，`-c output.csv`或`-j output.json`，两者二选一。
- 可选参数：使用`-w`选择输出带rawbow信息，rawbow特征列信息仅用于流量打标签进行人工校验。
- 对特征提取以后的CSV文件进行数据过滤：
  - 过滤总packets数小于16以下的流（特征行）。
  - 删除flow_id、src_ip和dst_ip列。
可以使用data目录下的脚本快速处理：`feature_filter.py`，需要指定config.yaml文件路径和数据文件夹路径。读取config配置文件中的过滤条件：`columns_to_remove、filter_packets`。如果特征提取时选择了输出rawbow信息，那么在数据过滤时需要增加对这一列的过滤操作（在columns_to_remove的列表中增加354），模型训练和验证时不支持rawbow特征列的输入。在线推理模式默认会过滤掉rawbow特征列。

    ```bash
    python /opt/KDADK-TOOLS/data/feature_filter.py /opt/KDADK-TOOLS/src/config.yaml /path/to/csv_data_folder
    ```

### 4. 在线推理 (-r)

- 参数：`-r config.yaml`
- 功能：读取[config配置](#7-config配置)文件，调用推理模块，执行在线推理任务，包括特征提取、数据过滤、特征推理。
- config参数：`model_path_onnx、scaler_path_json、onnx_log_level、columns_to_remove、filter_packets`。
- 输入数据源：
  - `-p xxx.pcap`：
      - 从pcap文件读取，支持多个-p指定输入pcap文件，可以指定多个pcap文件输入。
      - 使用`-m 0/1/2`指定使用的推理模式，其中0代表单flow模式, 1代表批处理模式, 2代表文件模式，默认为批处理模式。
      - 输出分类报告及分类结果，分类结果数据输出需要指定文件路径，分类报告保存在默认路径。
  - `-i interface`：从网口实时抓取流量进行特征提取及推理；可选输出分类结果；网口可以使用-l参数来进行查询。
- 输出：将分类结果输出到文件中，`-c output.csv`或`-j output.json`，两者二选一。

### 5. 查看网口 (-l)

- 参数：`-l`
- 功能：列出当前服务器网口信息。

### 6. 帮助信息(-h)

- 参数：`-h`
- 功能：查看参数使用帮助信息。

### 7. config配置
>
> ❗**须知：** 
> 所有路径支持绝对路径和相对路径，**推荐使用绝对路径**，在使用相对路径时默认从当前仓库根目录下算起。

- **`training_data_paths`**：**模型训练**数据输入，每一类数据置于一个数组中，不同数组代表不同类别，一类数据可以存放多个csv文件。

```bash
training_data_paths:
- [
  'data/bilibili/csv/bilibili_20250616_204043_10h_android_16.csv',
  'data/bilibili/csv/bilibili_20250716_101749_5h_android_18.csv',
  'data/bilibili/csv/bilibili_20250717_191237_50400_android_17.csv',
  'data/bilibili/csv/bilibili_20250718_101704_25200_android_19.csv'
]
- [
  'data/wenxiaoyan/csv/wenxiaoyan_20250726_172552_129600_android_16.csv',
  'data/wenxiaoyan/csv/wenxiaoyan_20250804_204806_144000_android_17.csv',
  'data/wenxiaoyan/csv/wenxiaoyan_20250804_205024_144000_android_18.csv',
  'data/wenxiaoyan/csv/wenxiaoyan_20250804_205029_144000_android_19.csv'
]
```

- **`model_path_pkl`**：训练后的模型权重输出路径，同时也是模型评估（python）的输入路径。

```bash
model_path_pkl: result/model_classifier_2.pkl
```

- **`scaler_path_pkl`**：训练后的标准化器权重输出路径，同时也是模型评估（python）的输入路径。

```bash
scaler_path_pkl: result/scaler_2.pkl
```

- **`model_path_onnx`**：训练后的模型权重输出路径，同时也是模型评估（C++）的输入路径。

```bash
model_path_onnx: result/model_classifier_2.onnx
```

- **`scaler_path_json`**：训练后的标准化器权重输出路径，同时也是模型评估（C++）的输入路径。

```bash
scaler_path_json: result/scaler_2.json
```

- **`evaluation_data_paths`**：**模型评估**数据输入，每一类数据置于一个数组中，不同数组代表不同类别，一类数据可以存放多个csv文件。

```bash
evaluation_data_paths:
  - ['data/bilibili/csv/bilibili_20250721_141827_21600_android_20.csv']
  - ['data/wenxiaoyan/csv/wenxiaoyan_20250804_205038_144000_android_20.csv']
```

- **`output_dir`**：模型评估结果输出文件夹。

```bash
output_dir: result
```

- **`classification_report_file`**：流分类报告结果路径。

```bash
classification_report_file: result/classification_report_2.txt
```

- **`predictions_detail_file`**：流分类详细结果路径。

```bash
predictions_detail_file: result/predictions_detail_2.csv
```

- **`onnx_log_level`**：ONNX运行时配置。

```bash
onnx_log_level: WARNING
# VERBOSE   详细信息
# INFO      一般信息
# WARNING   警告
# ERROR     错误
# FATAL     致命错误
```

- **`columns_to_remove`**：特征推理前对特征提取的结果进行过滤，列表中的数字表示特征列的下标减1，默认为0、1、2，其中第0列是flow_id,第1列是src_ip,第2列是dst_ip。用户可以自行调整，但是需要保证模型训练和模型推理时保持一致。

```bash
columns_to_remove: [0, 1, 2]
```

- **`filter_packets`**：特征推理前对特征提取的结果进行过滤，这里需要对send_packet_nums和receive_packet_nums之和小于filter_packets特征行进行过滤，默认值为16。用户可以自行调整，但是需要保证模型训练和模型推理时保持一致。

```bash
filter_packets: 16
```

## 工具用法

本仓库提供两个工具供用户使用：流量采集工具和流量打标签工具，位于`KDADK-TOOLS/tools/`，下面分别介绍用法。

### 1. 流量采集工具

流量采集工具位于`tools/capture/`目录下，有以下几个文件。

- `capture_win.ps1`           windows环境下使用wireshark来抓包，确保使用前安装wireshark，具体用法可以参考 `./capture_win.ps1 -h`。
- `capture_linux.sh`          linux环境下使用tcpdump来抓包，确保使用前安装tcpdump，具体用法可以参考 `sh capture_linux.sh -h`。
- `capture_linux_docker.sh`   linux环境下在docker容器中使用tcpdump来抓包，确保docker环境正确，具体用法可以参考 `sh capture_linux_docker.sh -h`。

在linux环境下使用`capture_linux.sh`来进行示范：

```bash
sh capture_linux.sh -d 60 -n input -i eth0
```
其中 `-d 60` 表示抓包时间为60秒；`-n input` 表示输出文件名为`input.pcap`，具体路径可以参考终端输出信息；`i eth0`表示从eth0网口抓取流量，使用-l来查看当前机器可用网口（up为可用）。


### 2. 流量打标签工具

流量打标签工具位于`tools/labeling/`目录下，有以下几个文件：

- labeling.py： 打标签工具，具体用法可以参考下述描述。

```bash
python labeling.py -h
```

出现下述回显：

```bash
usage: labeling.py [-h] [--mode {train,inference}] [--config CONFIG]

打标签工具

options:
  -h, --help            show this help message and exit
  --mode {train,inference}
                        运行模式: train(训练) 或 inference(推理)
  --config CONFIG       配置文件路径 (默认: config_label.yaml)

使用示例:
  python labeling.py --mode train                    # 使用默认配置文件训练
  python labeling.py --mode inference                # 使用默认配置文件推理，默认使用每个文件夹下的一个csv文件
  python labeling.py --mode train --config ./config_label.yaml  # 指定配置文件
```

- config_label.yaml： 配置文件，可以配置打标签工具具体参数。

> ❗**须知：** 
> 在inference推理时限定只输入一个csv特征文件，用户可以根据自己的需要来修改推理源码中的`inference=False`入参来取消这一限定。

```bash
# 流量打标签工具训练配置
train:
# 参与训练的数据，每个路径下可以有多个特征提取csv文件，同时支持字典模式，可以配置path和name
  data_paths: [
    'data/youku/csv',
    'data/yuanbao/csv',
    'data/wymusic/csv'
  ]

# data_paths: [
# {'path': 'data/youku/csv', 'name': 'video'}
# {'path': 'data/yuanbao/csv', 'name': 'text'}
# {'path': 'data/wymusic/csv', 'name': 'music'}
# ]

# 当前训练模型权重名
  model_name: "labeling_train"

# 当前数据集训练聚类数量，限定聚类数大于1  
  n_clusters: 3

# 数据集划分 30%用来测试，70%用来训练，输入范围为(0,1)
  test_size: 0.3

# 输出结果保存位置，输出文件有模型权重及标准化器权重
  results_dir: results        

# 流量打标签工具推理配置
inference:

# 参与推理的数据，每个路径下可以有多个特征提取csv文件，同时支持字典模式，可以配置path和name
  data_paths: [
    'data/youku/csv',
    'data/yuanbao/csv',
    'data/wymusic/csv'
  ]

# 加载预训练模型权重路径
  model_path: "results/labeling_train_model.pkl"

# 当前加载预训练标准化器权重路径  
  scaler_path: "results/labeling_train_scaler.pkl"

# 输出结果保存位置
  output_file: "output/output.csv"  

```

- results： 打标签工具结果输出位置，可在配置文件中修改。
- output： 聚类预测结果输出文件夹，可在配置文件中修改。