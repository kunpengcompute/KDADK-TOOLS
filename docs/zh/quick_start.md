# 快速入门
## 1. 快速上手
先进入到`demo/build`目录下
```
cd demo/build
```
（1） 特征提取
```
# 特征提取 - 输入为采集的流量数据pcap，输出为CSV格式的特征数据
./kdadk_demo -f input.pcap -c output.csv
```
回显如下：
```
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

- 注：对特征提取以后的CSV文件进行过滤：
  - 过滤总packets数小于16以下的流（特征行）
  - 删除flow_id、src_ip和dst_ip列
可以使用data目录下的脚本快速处理：`feature_filter.py`。会读取config配置文件中的过滤条件：`columns_to_remove、filter_packets`

```
cd ../../
cd data/
python feature_filter.py ../src/config.yaml /path/to/data
```

（2）模型训练
在config.yaml中配置模型训练所需要的csv文件路径

```
./kdadk_demo -t ../../src/config.yaml
```

（3）模型验证
在config.yaml中配置模型验证所需要的csv文件路径
```
./kdadk_demo -e ../../src/config.yaml
```

（4）在线推理
在config.yaml中配置训练好的模型权重路径，保持过滤条件和模型训练时一致

（4-1）多个PCAP文件，输出特征及预测结果的CSV文件
```
./kdadk_demo -r ../../src/config.yaml -p file1.pcap -p file2.pcap -m 1 -c result.csv
```

（4-2） 网口实时抓包，输出特征及预测结果的CSV文件
```
./kdadk_demo -r ../../src/config.yaml -i eth0 -c result.csv
```

接口信息可以通过以下命令查询
```
./kdadk_demo -l
```