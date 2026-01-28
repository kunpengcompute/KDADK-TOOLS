[TOC]

# KDADK-TOOLS
## 1. 项目介绍
KDADK-APPID是鲲鹏数据分析开发套件的网络流分类库，包含流汇聚、协议检测、特征计算、AI引擎和训练工具包，针对鲲鹏特性进行极致亲和优化，在具备对明文流量分析能力的同时，亦能对加密流量进行实时分析，弥补传统基于规则检测的DPI方案无法分析加密流量的短板，为解决业界普遍难题提供端到端参考方案。
KDADK-TOOLS是基于网络流分类库接口实现的demo示例，提供了特征提取及推理相关接口的使用示例；同时提供了一些工具用于流量采集、特征过滤、模型训练及自适应打标签。

## 2. 版本说明
**表1** 版本说明
| KDADK-TOOLS  | 特性  |
| ------------ | ------------ |
| 1.1.0  | 鲲鹏数据分析开发套件的网络流分类示例 |

## 3. 环境部署
KDADK-TOOLS当前适配的处理器和操作系统为鲲鹏920新型号处理器，openEuler操作系统，若您在使用过程中遇到问题，请先检查使用的环境是否在已验证的环境范围内。

**表2** KDADK-TOOLS已验证环境
| 操作系统  | CPU类型  |
| ------------ | ------------ |
| openEuler 22.03 LTS SP4  | 鲲鹏920新型号处理器  |
| openEuler 24.03 LTS SP2  | 鲲鹏920新型号处理器  |

**表3** 软件要求
| 软件名称| 版本 |
| --- | --- |
| GCC | 10.3.1 |
| CMake | 3.22.0 |
| yaml-cpp | 0.8.0 |
| onnxruntime| 1.22.0 |
| Python | 3.13 |

### 3.1 依赖准备
#### 3.1.1 安装yaml-cpp
[这里](https://github.com/jbeder/yaml-cpp/releases/tag/0.8.0)下载源码包

将下载好的源码放在`KDADK-TOOLS/util/`文件夹下，进到`KDADK-TOOLS/util/yaml-cpp`

```
# 进到yaml-cpp目录下
cd yaml-cpp

# 创建build文件夹
mkdir -p build

# 进入build
cd build

# cmake构建
cmake .. -DYAML_BUILD_SHARED_LIBS=ON

# make编译
make -j32

# install
make install
```
出现下述回显即为成功：

```
...
-- Installing: /usr/local/include/yaml-cpp/binary.h
-- Installing: /usr/local/include/yaml-cpp/anchor.h
-- Installing: /usr/local/include/yaml-cpp/eventhandler.h
-- Installing: /usr/local/include/yaml-cpp/emitterstyle.h
-- Up-to-date: /usr/local/include/yaml-cpp/contrib
-- Installing: /usr/local/include/yaml-cpp/contrib/graphbuilder.h
-- Installing: /usr/local/include/yaml-cpp/contrib/anchordict.h
-- Installing: /usr/local/include/yaml-cpp/stlemitter.h
-- Installing: /usr/local/include/yaml-cpp/ostream_wrapper.h
-- Installing: /usr/local/lib64/cmake/yaml-cpp/yaml-cpp-targets.cmake
-- Installing: /usr/local/lib64/cmake/yaml-cpp/yaml-cpp-targets-noconfig.cmake
-- Installing: /usr/local/lib64/cmake/yaml-cpp/yaml-cpp-config.cmake
-- Installing: /usr/local/lib64/cmake/yaml-cpp/yaml-cpp-config-version.cmake
-- Installing: /usr/local/lib64/pkgconfig/yaml-cpp.pc
```

#### 3.1.2 安装onnxruntime
[这里](https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-aarch64-1.22.0.tgz)下载源码包

将下载好的源码放在`KDADK-TOOLS/util/`文件夹下

```
# 解压
tar -xzf onnxruntime-linux-aarch64-1.22.0.tgz

# 设置环境变量
export ONNXRUNTIME_HOME=/path/to/your/onnxruntime
export LD_LIBRARY_PATH=$ONNXRUNTIME_HOME/lib:$LD_LIBRARY_PATH
```
#### 3.1.3 配置python环境

在`src/py/packages.txt`文件中记录了离线训练需要的python环境依赖，可以执行下述命令安装python环境依赖
```
pip install -r src/py/packages.txt
```
#### 3.1.4 准备特征提取和推理库

将下载好的动态库放到`KDADK-TOOLS/lib/`文件夹下

```
[root@master KDADK-TOOLS]# mkdir lib
[root@master KDADK-TOOLS]# cd lib
[root@master lib]# ls
libKDADK_feature_extract.so  libKDADK_inference.so
```
### 3.2 编译指导
进入到KDADK-TOOLS/demo执行下述命令

```
mkdir build
cd build
cmake ..
make
```
编译得到可执行的二进制程序：`kdadk_demo`。具体使用方法可以参考[使用说明](#kdadk_demo使用说明)。

## 4. 接口说明
### 4.1 特征提取接口

#### extractor_init

**功能**
创建并初始化特征提取器。
**函数接口**

````
feature_extractor *extractor_init(int link_type);
````

**参数**

* `link_type`： 链路层类型，支持`DLT_EN10MB(1)`、`DLT_LINUX_SLL(113)`

**返回值**

* 特征提取器句柄，失败返回NULL

#### extractor_destroy

**功能**
销毁特征提取器.
**函数接口**

````
void extractor_destroy(feature_extractor *extractor);
````

**参数**

* `extractor `：  特征提取器句柄

**返回值**

* 无

#### process_packet

**功能**
处理单个数据包，进行packet的协议解析及单个流的特征提取。
**函数接口**

````
int process_packet(feature_extractor *extractor, const unsigned char *packet, uint32_t packet_len,
                   const struct timeval *ts, feature_vector *features, int *has_feature);
````

**参数**

* `extractor `：  特征提取器句柄
* `packet`：  数据包指针
* `packet_len`：  数据包长度
* `ts`： 时间戳
* `features`：  输出特征向量（单个流）
* `has_feature`： 输出标志，表示是否有特征提取（流结束才会进行特征提取）

**返回值**

* 状态码：EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败

#### process_packet_with_rawbow

**功能**
处理单个数据包（提取特征向量和rawbow），进行packet的协议解析及单个流的特征提取，包含rawbow。
**函数接口**

````
int process_packet_with_rawbow(feature_extractor *extractor, const unsigned char *packet, uint32_t packet_len,
                               const struct timeval *ts, feature_vector *features, rawbow *rawbows, int *has_feature);
````

**参数**

* `extractor `：  特征提取器句柄
* `packet`：  数据包指针
* `packet_len`：  数据包长度
* `ts`： 时间戳
* `features`：  输出特征向量（单个流）
* `rawbows`： 输出rawbow（单个流）
* `has_feature`： 输出标志，表示是否有特征提取（流结束才会进行特征提取）

**返回值**

* 状态码：EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败

#### extractor_finalize

**功能**
处理所有剩余的流（仅提取特征向量）。
**函数接口**

````
int extractor_finalize(feature_extractor *extractor, feature_vector_list *features);
````

**参数**

* `extractor `：  特征提取器句柄
* `features`：   输出特征向量列表

**返回值**

* 状态码：EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败

#### extractor_finalize_with_rawbow

**功能**
处理所有剩余的流（提取特征向量和rawbow）。
**函数接口**

````
int extractor_finalize_with_rawbow(feature_extractor *extractor, feature_vector_list *features, rawbow_list *rawbows);
````

**参数**

* `extractor `：  特征提取器句柄
* `features`：   输出特征向量列表
* `rawbows`： 输出rawbow列表

**返回值**

* 状态码：EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败

#### extractor_get_statistics

**功能**
获取统计信息。
**函数接口**

````
int extractor_get_statistics(feature_extractor *extractor, statistics *stats);
````

**参数**

* `extractor `：  特征提取器句柄
* `stats`：   stats 输出统计信息

**返回值**

* 状态码：EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败

#### extractor_reset_statistics

**功能**
重置统计信息。
**函数接口**

````
void extractor_reset_statistics(feature_extractor *extractor);
````

**参数**

* `extractor `：  特征提取器句柄
* `stats`：   stats 输出统计信息

**返回值**

* 无

### 4.2 特征推理接口

#### inference_init

**功能**
创建并初始化推理引擎
**函数接口**

````
inference_engine *inference_init(const inference_config *config);
````

**参数**

* `config_file`：推理配置（包含配置文件路径）

**返回值**

* 推理引擎句柄，失败返回NULL。

#### inference_destroy

**功能**
销毁推理引擎
**函数接口**

````
void inference_destroy(inference_engine *engine);
````

**参数**

* `engine`：推理引擎句柄

**返回值**

* 无

#### inference_predict

**功能**
批量推理
**函数接口**

````
int inference_predict(inference_engine *engine, const feature_vector_list *features, inference_result *result);
````

**参数**

* `engine`：推理引擎句柄
* `features`： 特征向量列表
* `result`：输出推理结果

**返回值**

* 状态码：INFERENCE_SUCCESS表示成功，INFERENCE_ERROR表示失败


<a id="kdadk_demo使用说明"></a>
## 5. 使用说明
kdadk_demo可以使用-h查看程序功能，如下所示：

```
[root@master build]# ./kdadk_demo -h
用法: ./kdadk_demo [选项]

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
  -w                      输出是否带有rawbow (仅用于 -f 特征提取模式)
  -l                      列出当前机器可用网口
  -h                      显示帮助信息

说明:
  -c 和 -j 选项只能选择其中一个，分别指定CSV或JSON格式的输出文件
  -c 选项的文件必须以 .csv 结尾
  -j 选项的文件必须以 .json 结尾

示例:
  # 特征提取 - CSV格式
  ./kdadk_demo -f input.pcap -c output.csv

  # 特征提取 - JSON格式，带rawbow
  ./kdadk_demo -f input.pcap -j output.json -w

  # 在线推理 - 多个PCAP文件，输出CSV
  ./kdadk_demo -r config.yaml -p file1.pcap -p file2.pcap -m 1 -c result.csv

  # 在线推理 - 网口实时抓包，输出JSON
  ./kdadk_demo -r config.yaml -i eth0 -j result.json

  # 列出可用网口
  ./kdadk_demo -l
```
其中核心功能模块有：模型训练(-t)、模型验证(-e)、特征提取(-f)、在线推理(-r)。

### 5.1 模型训练 (-t)

- 参数：`-t config.yaml`
- 功能：读取配置文件，调用模型训练脚本（Python），执行模型训练任务。[config配置说明](#config配置)
- config参数：`training_data_paths、model_path_pkl、scaler_path_pkl、model_path_onnx、scaler_path_json`

### 5.2 模型验证 (-e)

- 参数：`-e config.yaml`
- 功能：读取配置文件，调用模型验证脚本（Python），执行模型验证。[config配置说明](#config配置)
- config参数：`evaluation_data_paths、model_path_pkl、scaler_path_pkl、output_dir、classification_report_file、predictions_detail_file`
- 注：对特征提取以后的CSV文件进行过滤：
  - 过滤总packets数小于16以下的流（特征行）
  - 删除flow_id、src_ip和dst_ip列
可以使用data目录下的脚本快速处理：`feature_filter.py`。会读取config配置文件中的过滤条件：`columns_to_remove、filter_packets`

```
python feature_filter.py ../src/config.yaml /path/to/data
```
### 5.3 特征提取 (-f)

- 参数：`-f xxx.pcap -c output.csv`或`-f xxx.pcap -j output.json`
- 功能：输入pcap离线网络流量，进行协议解析及特征提取。
- 输出：将特征提取结果保存在文件中，`-c output.csv`或`-j output.json`，两者二选一
- 可选参数：使用`-w`选择输出是否带rawbow信息

### 5.4 在线推理 (-r)

- 参数：`-r config.yaml`
- 功能：读取配置文件，调用推理模块，执行在线推理任务，包括特征提取、数据过滤、特征推理。[config配置说明](#config配置)
- config参数：`model_path_onnx、scaler_path_json、onnx_log_level、columns_to_remove、filter_packets`
- 输入数据源：
  - `-p xxx.pcap`：
	  - 从pcap文件读取，支持多个-p指定输入多个pcap文件
      - 使用`-m 0/1/2`指定使用的推理模式，其中0代表单flow模式, 1代表批处理模式, 2代表文件模式，默认为批处理模式
      - 输出分类报告及分类结果，pcap数据输出需要指定文件路径
  - `-i interface`：从网口实时抓取流量进行特征提取及推理；可选输出分类结果；网口可以使用-l参数来进行查询
- 输出：将分类结果输出到文件中，`-c output.csv`或`-j output.json`，两者二选一

### 5.5 查看网口 (-l)

- 参数：`-l`
- 功能：列出当前机器网口信息

### 5.6 帮助信息(-h)

- 参数：`-h`
- 功能：查看参数使用帮助信息

<a id="config配置"></a>
### 5.7 config配置
- **`training_data_paths`**：**模型训练**数据输入，每一类数据置于一个数组中，不同数组代表不同类别，一类数据可以存放多个csv文件

```
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
- **`model_path_pkl`**：训练后的模型权重输出路径，同时也是模型评估（python）的输入路径

```
model_path_pkl: result/model_classifier_2.pkl
```
- **`scaler_path_pkl`**：训练后的标准化器权重输出路径，同时也是模型评估（python）的输入路径

```
scaler_path_pkl: result/scaler_2.pkl
```
- **`model_path_onnx`**：训练后的模型权重输出路径，同时也是模型评估（C++）的输入路径

```
model_path_onnx: result/model_classifier_2.onnx
```
- **`scaler_path_json`**：训练后的标准化器权重输出路径，同时也是模型评估（C++）的输入路径

```
scaler_path_json: result/scaler_2.json
```
- **`evaluation_data_paths`**：**模型评估**数据输入，每一类数据置于一个数组中，不同数组代表不同类别，一类数据可以存放多个csv文件

```
evaluation_data_paths:
  - ['data/bilibili/csv/bilibili_20250721_141827_21600_android_20.csv']
  - ['data/wenxiaoyan/csv/wenxiaoyan_20250804_205038_144000_android_20.csv']
```
- **`output_dir`**：模型评估结果输出文件夹

```
output_dir: result
```
- **`classification_report_file`**：流分类报告结果路径

```
classification_report_file: result/classification_report_2.txt
```
- **`predictions_detail_file`**：流分类详细结果路径

```
predictions_detail_file: result/predictions_detail_2.csv
```
- **`onnx_log_level`**：ONNX运行时配置。

```
onnx_log_level: WARNING
# VERBOSE   详细信息
# INFO      一般信息
# WARNING   警告
# ERROR     错误
# FATAL     致命错误
```
- **`columns_to_remove`**：特征推理前对特征提取的结果进行过滤，列表中的数字表示特征列的下标减1，默认为0、1、2，其中第0列是flow_id,第1列是src_ip,第2列是dst_ip。用户可以自行调整，但是需要保证模型训练和模型推理时保持一致。

```
columns_to_remove: [0, 1, 2]
```
- **`filter_packets`**：特征推理前对特征提取的结果进行过滤，这里需要对send_packet_nums和receive_packet_nums之和小于filter_packets特征行进行过滤，默认值为16。用户可以自行调整，但是需要保证模型训练和模型推理时保持一致。

```
filter_packets: 16
```
❗注：所有路径支持绝对路径和相对路径，**推荐使用绝对路径**，在使用相对路径时默认从当前仓库根目录下算起。

## 6. 工具用法
本仓库提供两个工具供用户使用：流量采集工具和流量打标签工具，位于`/tools/`，下面分别介绍用法。

### 6.1 流量采集工具
`/tools/capture/`

- `capture_win.ps1`           windows环境下使用wireshark来抓包，确保使用前安装wireshark，具体用法可以参考 `./capture_win.ps1 -h`
- `capture_linux.sh`          linux环境下使用tcpdump来抓包，确保使用前安装tcpdump，具体用法可以参考 `./capture_linux.sh -h`
- `capture_linux_docker.sh`   linux环境下在docker容器中使用tcpdump来抓包，确保docker环境正确，具体用法可以参考 `./capture_linux_docker.sh -h`

### 6.2 流量打标签工具
`/tools/labeling/`
有以下几个文件：
- labeling.py                 -- 打标签工具，具体用法可以参考下述描述


```
python labeling.py -h

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

- config_label.yaml           -- 配置文件，可以配置打标签工具具体参数

```
# 流量打标签工具训练配置
train:
  # 参与训练的数据，每个路径下可以有多个特征提取csv文件，同时支持字典模式，可以配置path和name
  data_paths: [
    'data/youku/csv',
    'data/yuanbao/csv',
    'data/wymusic/csv'
  ]
  #   data_paths: [
  #     {'path': 'data/youku/csv', 'name': 'video'},
  #     {'path': 'data/yuanbao/csv', 'name': 'text'},
  #     {'path': 'data/wymusic/csv', 'name': 'music'}
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
- results                     -- 打标签工具结果输出位置，可在配置文件中修改
- output                      -- 聚类预测结果输出文件夹，可在配置文件中修改


- **注：在inference推理时限定只输入一个csv特征文件，用户可以根据自己的需要来修改inferenceing源码中的`inference=False`入参来取消这一限定。**

##  7. 其他事项
如果使用过程中有任何问题，或者需要反馈特性需求和bug报告，可以提交issues联系我们，具体贡献方法可参考[这里](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md)。