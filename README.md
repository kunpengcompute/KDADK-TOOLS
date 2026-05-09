# KDADK-TOOLS

## 最新消息

\[2026-03-30\]: 发布KDADK-TOOLS 1.0.0。更新了特征提取及特征推理接口头文件及软件包，提供了鲲鹏BoostKit智能数据分析开发套件的网络流分类参考示例。

## 项目介绍
KDADK-APPID是鲲鹏BoostKit智能数据分析开发套件的网络流分类库，包含流汇聚、协议检测、特征计算、AI引擎和训练工具包，针对鲲鹏特性进行极致亲和优化，在具备对明文流量分析能力的同时，亦能对加密流量进行实时分析，弥补传统基于规则检测的DPI方案无法分析加密流量的短板，为解决业界普遍难题提供端到端参考方案。

KDADK-TOOLS是基于网络流分类库接口实现的demo示例，提供了特征提取及推理相关接口的使用示例；同时提供了一些工具用于流量采集、特征过滤、模型训练及自适应打标签。

## 特性介绍
|特性名称|特性介绍
|--|--|
| 加密流量处理 | 加密流量处理特性提供了特征提取、特征推理接口头文件及软件包，同时提供了鲲鹏BoostKit智能数据分析开发套件的网络流分类参考示例。 |

## 目录结构
项目目录结构如下：
```
|—— data                                                      # 特征提取数据处理脚本工具
│   ├── feature_filter.py                                     # 特征过滤脚本
|—— demo                                                      # 网络流分类库接口实现的demo示例源码
|—— docs                                                      # 文档资料目录
│   └── zh                                                    # 中文文档目录
│       ├── quick_start.md                                    # 快速入门
│       ├── release_notes.md                                  # 版本说明书
│       ├── installation_guide.md                             # 安装指南
│       ├── user_guide.md                                     # 用户指南
│       ├── api_reference.md                                  # API参考
│       ├── faq.md                                            # 常见问题
|—— include                                                   # 特征提取及特征推理接口头文件
│   ├── kdadk_feature_extract.h                               # 特征提取接口头文件
│   ├── kdadk_inference.h                                     # 特征推理接口头文件
|—— src                                                       # 模型训练及模型验证工具，config.yaml配置文件
│   └── py                                                    # 模型训练和模型验证python脚本目录
│   ├── config.yaml                                           # 配置文件
|—— tools                                                     # 流量采集和流量打标签工具
│   ├── capture/                                              # 流量采集工具目录
│   ├── labeling/                                             # 流量打标签工具目录
|—— util                                                      # 环境部署依赖包安装目录
|—— README.md                                                 # KDADK-TOOLS项目介绍
```

## 版本说明
每个版本的特性变更详细信息，具体请参见《[版本说明书](docs/zh/release_notes.md)》。


## 环境部署
KDADK-TOOLS当前适配的处理器和操作系统为鲲鹏920新型号处理器，openEuler操作系统，若您在使用过程中遇到问题，请先检查使用的环境是否在已验证的环境范围内。

**表1** KDADK-TOOLS已验证环境
| 操作系统  | CPU类型  |
| ------------ | ------------ |
| openEuler 22.03 LTS SP4  | 鲲鹏920新型号处理器  |
| openEuler 24.03 LTS SP2  | 鲲鹏920新型号处理器  |

**表2** 软件要求
| 软件名称| 版本 |
| --- | --- |
| GCC | 10.3.1 |
| CMake | 3.22.0 |
| yaml-cpp | 0.8.0 |
| onnxruntime| 1.22.0 |
| Python | 3.9.9及以上 |
| KDADK-APPID | 1.0.0 |

安装方法可以参考《[安装指南](docs/zh/installation_guide.md)》。

编译得到可执行的二进制程序：`kdadk_demo`。

## 快速上手
KDADK-TOOLS的快速入门通过使用kdadk_demo来实现，具体请参见《[快速入门](docs/zh/quick_start.md)》。

## 学习文档
本特性提供以下学习文档可供参考。
| 文档 | 介绍|
| -- | -- |
| [快速入门](docs/zh/quick_start.md) | 提供KDADK-TOOLS的demo示例快速使用方法。 |
| [版本说明书](docs/zh/release_notes.md)| 提供KDADK-TOOLS的版本更新说明。 |
| [安装指南](docs/zh/installation_guide.md) | 提供KDADK-TOOLS的demo示例的依赖包安装及编译方法。 |
| [用户指南](docs/zh/user_guide.md)| 提供KDADK-TOOLS的demo示例、流量采集工具及流量打标签工具的详细用法。 |
| [API参考](docs/zh/api_reference.md) | 提供KDADK-TOOLS的特征提取和特征推理的接口说明。 |
| [常见问题](docs/zh/faq.md) | 列举KDADK-TOOLS的一些常见问题。 |

## 贡献声明
如果使用过程中有任何问题，或者需要反馈特性需求和bug报告，可以提交issues联系我们，具体贡献方法可参考[这里](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md)。

## 免责声明
致KDADK-TOOLS使用者
- 本项目仅提供参考示例及工具，使用者需自行承担使用风险，并理解以下内容：
    - 数据处理及删除：用户在使用本工具过程中产生的数据属于用户责任范畴。建议用户在使用完毕后及时删除相关数据，以防信息泄露。
    - 数据保密与传播：使用者了解并同意不得将通过本工具产生的数据随意外发或传播。对于由此产生的信息泄露、数据泄露或其他不良后果，本工具及其开发者概不负责。
    - 用户输入安全性：用户需自行保证输入的命令行的安全性，并承担因输入不当而导致的任何安全风险或损失。对于输入命令行不当所导致的问题，本工具及其开发者概不负责。
- 免责声明范围：本免责声明适用于所有使用本工具的个人或实体。不建议将此工具用于任何商业用途。使用本工具即表示您同意并接受本声明的内容，并愿意承担因使用该功能而产生的风险和责任，如有异议请停止使用本工具。
- 在使用本工具之前，请谨慎阅读并理解以上免责声明的内容。对于使用本工具所产生的任何问题或疑问，请及时联系开发者。

## 许可证书
本项目采用BSD 3-Clause License许可证。详见[LICENSE](LICENSE)。

本项目的文档适用CC-BY 4.0许可证，具体请参见[LICENSE](docs/LICENSE)文件。