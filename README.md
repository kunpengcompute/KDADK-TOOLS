[TOC]

# KDADK-TOOLS
## 1. 项目介绍
KDADK-TOOLS是鲲鹏数据分析开发套件的网络流分类库，包含流汇聚、协议检测、特征计算、规则+AI双引擎和训练工具包，针对鲲鹏特性进行极致亲和优化，在具备对明文流量分析能力的同时，亦能对加密流量进行实时分析，弥补传统基于规则检测的DPI方案无法分析加密流量的短板，为解决业界普遍难题提供端到端参考方案。

## 2. 版本说明
**表1** 版本说明
| KDADK-TOOLS  | 特性  |
| ------------ | ------------ |
| 1.0.1  | 鲲鹏数据分析开发套件的网络流分类库 |

## 3. 环境部署
KDADK-TOOLS当前适配的处理器和操作系统为鲲鹏920系列处理器，openEuler 22.03操作系统，若您在使用过程中遇到问题，请先检查使用的环境是否在已验证的环境范围内。

**表2** KDADK-TOOLS已验证环境
| 操作系统  | CPU类型  |
| ------------ | ------------ |
| openEuler 22.03 LTS SP4  | 鲲鹏920系列处理器  |

**表3** 软件要求
| 软件名称| 版本 |
| --- | --- |
| GCC | 10.3.1及以上 |
| CMake | 3.22.0及以上 |
| yaml-cpp | 0.8.0 |
| onnxruntime| 1.22.0及以上|

- 注：还需准备python环境，相关软件包及版本可见`KDADK-TOOLS/src/py/packages.txt`

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
[这里](https://github.com/microsoft/onnxruntime/releases)下载源码包

将下载好的源码放在`KDADK-TOOLS/util/`文件夹下

```
# 解压
tar -xzf onnxruntime-linux-aarch64-1.22.0.tgz

# 设置环境变量
export ONNXRUNTIME_HOME=/path/to/your/onnxruntime
export LD_LIBRARY_PATH=$ONNXRUNTIME_HOME/lib:$LD_LIBRARY_PATH
```

#### 3.1.3 配置华为安全函数
[这里](https://gitee.com/openeuler/libboundscheck)下载源码包

将下载好的源码放在`KDADK-TOOLS/util/`文件夹下

```
# 进入到KDADK-TOOLS/util/libboundscheck/下
make CC=gcc

# 设置环境变量
export SECUREC_HOME=/path/to/your/libboundscheck
export LD_LIBRARY_PATH=$SECUREC_HOME/lib:$LD_LIBRARY_PATH
```
#### 3.1.4 配置python环境

在`src/py/packages.txt`文件中记录了离线训练需要的python环境依赖，可以执行下述命令安装python环境依赖
```
pip install -r src/py/packages.txt
```
## 4. 使用说明
KDADK-TOOLS的入口函数目前有四个接口，分别是特征提取接口、模型训练接口、模型评估接口（python）以及模型推理接口（C++）。如下所示：

```
[root@ceph1 output]# ./kdadk_appid -h
Usage:
  ./kdadk_appid -f <pcap_file>              Extract features from PCAP
  ./kdadk_appid -t <config.yaml>            Train model
  ./kdadk_appid -e <config.yaml>            Evaluate model
  ./kdadk_appid -i <config.yaml>            Run inference
  ./kdadk_appid -h                          Show this help
```
四个接口的使用方法分别如下。
- 注：output下的可执行文件以及动态链接库使用前需要添加执行权限。

### 4.1 特征提取接口
`./kdadk_appid -f <pcap_file>`
其中参数`<pcap_file>`为要进行特征提取操作的pcap文件地址，输出为csv特征数据，位于pcap文件同级目录下。

```
[root@ceph1 output]# ./kdadk_appid -f ../data/aiqiyi/aiqiyi_20250715_093827_5h_android_16.pcap 
Extracting features from: ../data/aiqiyi/aiqiyi_20250715_093827_5h_android_16.pcap
Total packets: 1158299, decode packets: 1157053
Feature extraction completed.
```
- 注：对特征提取以后的CSV文件进行过滤：
  - 过滤16个包以下的流
  - 去除src_ip和dst_ip两列

可以使用data目录下的两个脚本快速处理：`delete_flow_under_16.py、delete_first_two_column.py`。

```
python delete_flow_under_16.py

python delete_first_two_column.py
```
### 4.2 模型训练接口
`./kdadk_appid -t <config.yaml>`
其中参数`<config.yaml>`为配置模型训练所需csv文件的yaml文件地址，该接口采用调用随机森林训练python文件运行的方式执行。`config.yaml`默认在`src/`目录下。[config配置说明](#config配置)


```
(base) [root@ceph1 output]# ./kdadk_appid -t ../src/config.yaml 
Training model with: src/config.yaml
2025-11-25 14:56:02,099 - INFO - Loaded config file : ../src/config.yaml
2025-11-25 14:56:06,081 - INFO - Start training the model...
2025-11-25 14:56:51,269 - INFO - OOB Score: 0.9911258584767343
2025-11-25 14:56:51,283 - INFO - Top 30 features:
                     Feature  Importance
343      raw_bow_crc_hist_24    0.053819
164          receive_hdr_max    0.037204
320       raw_bow_crc_hist_1    0.036575
194  receive_hdr_hist_abs_11    0.030185
58              send_pld_max    0.028718
60              send_pld_var    0.028629
109        send_interval_min    0.026081
268     receive_interval_max    0.023495
347      raw_bow_crc_hist_28    0.022692
216          receive_pld_max    0.022460
166          receive_hdr_var    0.022332
110        send_interval_max    0.020868
1                   dst_port    0.019146
59             send_pld_mean    0.018439
161        receive_byte_nums    0.017982
322       raw_bow_crc_hist_3    0.017912
270     receive_interval_var    0.016703
196  receive_hdr_hist_abs_13    0.014535
165         receive_hdr_mean    0.014486
57              send_pld_min    0.014203
217         receive_pld_mean    0.014036
215          receive_pld_min    0.013714
325       raw_bow_crc_hist_6    0.013343
112        send_interval_var    0.013024
77       send_pld_hist_abs_0    0.013007
269    receive_interval_mean    0.011972
345      raw_bow_crc_hist_26    0.011506
111       send_interval_mean    0.011128
236   receive_pld_hist_abs_1    0.010101
218          receive_pld_var    0.009542
结果保存路径: KDADK-TOOLS/result
2025-11-25 14:56:51,363 - INFO - Model saved as result/model_classifier.pkl
2025-11-25 14:56:52,840 - INFO - Model saved as result/model_classifier.onnx
2025-11-25 14:56:52,843 - INFO - Scaler saved as result/scaler.pkl
StandardScaler 参数已导出到 result/scaler_params.json
```
### 4.3 模型评估接口（python）
`./kdadk_appid -e <config.yaml>`
其中参数`<config.yaml>`为配置模型评估所需csv文件的yaml文件地址，该接口采用调用随机森林评估python文件运行的方式执行。`config.yaml`默认在`src/`目录下。[config配置说明](#config配置)

```
(base) [root@ceph1 output]# ./kdadk_appid -e ../src/config.yaml 
Evaluating model with: ../src/config.yaml
2025-11-25 14:58:27,237 - INFO - Loaded config file : ../src/config.yaml
2025-11-25 14:58:29,022 - INFO - Model evaluation result:
              precision    recall  f1-score   support

           0     0.9976    0.9985    0.9981      3370
           1     0.9779    0.9892    0.9835      2591
           2     0.9958    0.9952    0.9955      5460
           3     0.9980    0.9955    0.9968      6470
           4     0.9912    0.9762    0.9837       925
           5     0.9978    0.9978    0.9978      8047

    accuracy                         0.9952     26863
   macro avg     0.9930    0.9921    0.9925     26863
weighted avg     0.9953    0.9952    0.9952     26863

2025-11-25 14:58:29,023 - INFO - Accuracy: 0.9952350817109035
2025-11-25 14:58:29,023 - INFO - Classification report saved to: result/classification_report_6.txt
2025-11-25 14:58:35,966 - INFO - Prediction results saved to: result/predictions_detail_6.csv
2025-11-25 14:58:35,966 - INFO - Total samples: 26863
2025-11-25 14:58:35,966 - INFO - Correct predictions: 26735/26863
```
### 4.4 模型评估接口（C++）
`./kdadk_appid -i <config.yaml>`
其中参数`<config.yaml>`为配置模型推理所需csv文件的yaml文件地址，该接口采用随机森林模型C++推理方式执行。`config.yaml`默认在`src/`目录下。[config配置说明](#config配置)

```
(base) [root@ceph1 output]# ./kdadk_appid -i ../src/config.yaml 
Running inference with: ../src/config.yaml
正在加载配置文件...
加载标准化器参数: /home/l00934292/KDADK-TOOLS/result/scaler_params.json
加载标准化器参数成功，特征数量: 351
加载和准备数据...
提示: 文件 data/bilibili/csv/bilibili_20250721_141827_21600_android_20.csv 所有数据行均有效
提示: 文件 data/aiqiyi/csv/aiqiyi_20250721_141719_21600_android_19.csv 所有数据行均有效
提示: 文件 data/tencent/csv/tencent_20250730_172737_57600_android_20.csv 所有数据行均有效
提示: 文件 data/wenxiaoyan/csv/wenxiaoyan_20250804_205038_144000_android_20.csv 所有数据行均有效
提示: 文件 data/doubao/csv/doubao_20250721_201921_216000_android_17.csv 所有数据行均有效
提示: 文件 data/yuanbao/csv/yuanbao_20250721_142339_216000_android_18.csv 所有数据行均有效
加载数据完成，样本数量: 26863
检测到 raw_bow 列，将在输出中保留
加载ONNX模型: result/model_classifier.onnx
进行预测...
ONNX推理：成功解析标签数量 = 26863
准确率: 0.994602
目录已存在: result
分类报告已保存到: result/classification_report_6.txt
详细预测结果已保存到: result/predictions_detail_6.csv

推理完成！结果已保存到 result 目录
Inference completed.
```
<a id="config配置"></a>
### 4.5 config配置
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
❗**注：所有路径支持绝对路径和相对路径，推荐使用绝对路径，在使用相对路径时默认从当前仓库根目录下算起。**
## 5. 工具用法
本仓库提供两个工具供用户使用：流量采集工具和流量打标签工具，位于`/tools/`，下面分别介绍用法。

### 5.1 流量采集工具
`/tools/capture/`

- `capture_win.ps1`           windows环境下使用wireshark来抓包，确保使用前安装wireshark，具体用法可以参考 `./capture_win.ps1 -h`
- `capture_linux.sh`          linux环境下使用tcpdump来抓包，确保使用前安装tcpdump，具体用法可以参考 `./capture_linux.sh -h`
- `capture_linux_docker.sh`   linux环境下在docker容器中使用tcpdump来抓包，确保docker环境正确，具体用法可以参考 `./capture_linux_docker.sh -h`

### 5.2 流量打标签工具
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
    '/home/l00934292/appid_data/youku/csv',
    '/home/l00934292/appid_data/yuanbao/csv',
    '/home/l00934292/appid_data/wymusic/csv'
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
    '/home/l00934292/appid_data/youku/csv',
    '/home/l00934292/appid_data/yuanbao/csv',
    '/home/l00934292/appid_data/wymusic/csv'
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

##  6. 其他事项
如果使用过程中有任何问题，或者需要反馈特性需求和bug报告，可以提交issues联系我们，具体贡献方法可参考[这里](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md)。