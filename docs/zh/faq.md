# 常见问题

## 特征提取时支持哪些协议类型？

目前特征提取时只支持TCP和UDP协议，其他协议的包会默认丢弃。在demo示例的在线推理模式中如果使用非TCP/UDP协议pcap数据（如ICMP协议数据）时，会在特征提取时把不符合要求的包都丢弃，例如：

```bash
[root@ceph1 KDADK-TOOLS]# /opt/KDADK-TOOLS/demo/build/kdadk_demo -r /opt/KDADK-TOOLS/src/config.yaml -m 0 -p /root/icmp_data.pcap -c result.csv
========== 在线推理模式 ==========
配置文件: /opt/KDADK-TOOLS/src/config.yaml
Loading config file: /opt/KDADK-TOOLS/src/config.yaml
model_path: /opt/KDADK-TOOLS/result/model_classifier_2.onnx
scaler_path: /opt/KDADK-TOOLS/result/scaler_params_2.json
filter_packets: 16
columns_to_remove: [0, 1, 2]
Loading ONNX model: /opt/KDADK-TOOLS/result/model_classifier_2.onnx
Loading scaler: /opt/KDADK-TOOLS/result/scaler_params_2.json
加载标准化器参数成功，特征数量: 351
Inference engine initialized successfully

========== PCAP文件推理模式 ==========
文件数量: 1
推理模式: 0 (0=单flow, 1=批处理, 2=文件)
输出文件: result.csv (格式: CSV)

处理文件 1/1: /root/icmp_data.pcap (Label=0)

======================= 性能统计 =======================
PCAP读取耗时:       0.0000 秒
特征提取耗时:       0.0003 秒
推理耗时:           0.0000 秒
文件IO耗时:         0.0000 秒
总耗时:             0.0003 秒 (特征提取 + 推理 + 文件IO)
端到端总耗时:       0.0018 秒 (包含PCAP读取等所有开销)
处理包数:               18 个
总数据量:           0.0017 MB
特征提取吞吐量:     6.2498 MB/s (    0.0488 Gbps)
核心处理吞吐量:     6.2498 MB/s (    0.0488 Gbps)
端到端吞吐量:       0.9222 MB/s (    0.0072 Gbps)
========================================================


过滤统计: 保留 0 个样本, 过滤掉 0 个样本
Inference engine destroyed
```

这里的输入是icmp_data.pcap数据，可以从性能统计看到处理的包数有18个，最终的过滤统计显示“保留 0 个样本, 过滤掉 0 个样本”，表示特征提取并没有提取到流数据，其中不符合要求的ICMP包全部被丢弃。
