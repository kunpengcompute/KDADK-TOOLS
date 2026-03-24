# 快速入门

KDADK-TOOLS提供了demo示例，通过使用可执行文件`kdadk_demo`来展示特征提取、模型训练、模型验证和在线推理的流程。详细使用细节请参考《[使用指南](user_guide.md#使用方法)》。

## 特征提取
使用 -f 参数指定为特征提取模式。 输入为采集的流量数据pcap，输出为CSV格式的特征数据。这里的输入数据input.pcap可以通过[流量采集工具](user_guide.md#1-流量采集工具)来进行采集。

```bash
/opt/KDADK-TOOLS/demo/build/kdadk_demo -f input.pcap -c output.csv
```

回显如下：

```bash
========== 特征提取模式 ==========
输入文件: input.pcap
输出文件: output.csv
输出rawbow: 否
File writer created: output.csv (format=CSV, mode=features_only)
处理完成:
  - 总包数: 2728091
  - 总流数: 14039
  - 输出文件: output.csv
File writer destroyed
```

- 对特征提取以后的CSV文件进行数据过滤：
  - 过滤总packets数小于16以下的流（特征行）
  - 删除flow_id、src_ip和dst_ip列
可以使用`data`目录下的脚本快速处理：`feature_filter.py`，需要指定配置文件路径和数据文件夹路径（特征提取输出的csv文件所在目录）。

    ```bash
    python /opt/KDADK-TOOLS/data/feature_filter.py /opt/KDADK-TOOLS/src/config.yaml /path/to/csv_data_folder
    ```

## 模型训练
使用 `-t` 参数指定为模型训练模式。在`src/config.yaml`中配置模型训练所需要的csv文件路径。可参考使用指南的[模型训练](user_guide.md#1-模型训练--t)章节。

```bash
/opt/KDADK-TOOLS/demo/build/kdadk_demo -t /opt/KDADK-TOOLS/src/config.yaml
```

## 模型验证
使用 `-e` 参数指定为模型验证模式。在`src/config.yaml`中配置模型验证所需要的csv文件路径。可参考使用指南的[模型验证](user_guide.md#2-模型验证--e)章节。

```bash
/opt/KDADK-TOOLS/demo/build/kdadk_demo -e /opt/KDADK-TOOLS/src/config.yaml
```

## 在线推理
使用 `-r` 参数指定为在线推理模式。在`src/config.yaml`中配置训练好的模型权重路径，保持过滤条件和模型训练时一致。可参考使用指南的[在线推理](user_guide.md#4-在线推理--r)章节。

在线推理模式支持两种数据输入方式：离线pcap数据和网口实时抓包数据，分别使用`-p`参数和`-i`参数指定。

    1. 输入为多个PCAP文件，输出特征及预测结果的CSV文件，使用-p参数指定pcap文件路径。

    ```bash
    /opt/KDADK-TOOLS/demo/build/kdadk_demo -r /opt/KDADK-TOOLS/src/config.yaml -p file1.pcap -p file2.pcap -m 1 -c result.csv
    ```

    2. 输入为网口实时抓包数据，输出特征及预测结果的CSV文件，使用-i参数指定网口名称。

    ```bash
    /opt/KDADK-TOOLS/demo/build/kdadk_demo -r /opt/KDADK-TOOLS/src/config.yaml -i eth0 -c result.csv
    ```

    网口信息可以通过以下命令查询。

    ```bash
    /opt/KDADK-TOOLS/demo/build/kdadk_demo -l
    ```
