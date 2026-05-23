# FAQs

## What Protocols Are Supported During Feature Extraction?

**Symptom**

What protocols are supported during feature extraction?

**Key Process and Cause Analysis**

Currently, only TCP and UDP are supported during feature extraction. The packets transferred over other protocols are discarded by default.

**Conclusion and Solution**

In the online inference mode of the demo, if the PCAP data of a non-TCP/UDP protocol (such as ICMP data) is used, the packets that do not meet the requirements will be discarded during feature extraction. The following is an example.

```bash
[root@ceph1 KDADK-TOOLS]# /opt/KDADK-TOOLS/demo/build/kdadk_demo -r /opt/KDADK-TOOLS/src/config.yaml -m 0 -p /root/icmp_data.pcap -c result.csv
========== Online inference mode ==========
Config file: /opt/KDADK-TOOLS/src/config.yaml
Loading config file: /opt/KDADK-TOOLS/src/config.yaml
model_path: /opt/KDADK-TOOLS/result/model_classifier_2.onnx
scaler_path: /opt/KDADK-TOOLS/result/scaler_params_2.json
filter_packets: 16
columns_to_remove: [0, 1, 2]
Loading ONNX model: /opt/KDADK-TOOLS/result/model_classifier_2.onnx
Loading scaler: /opt/KDADK-TOOLS/result/scaler_params_2.json
Scaler parameters loaded successfully. Number of features: 351
Inference engine initialized successfully

========== PCAP file inference mode ==========
Number of files: 1
Inference mode: 0 (0: single-flow; 1: batch processing; 2: file)
Output file: result.csv (format: CSV)

Processing file 1/1: /root/icmp_data.pcap (Label=0)

======================= Performance statistics =======================
PCAP file reading:              0.0000s
Feature extraction:             0.0003s
Inference:                      0.0000s
File I/O:                       0.0000s
Total duration:                 0.0003s (feature extraction + inference + file I/O)
E2E duration:                   0.0018s (including all overheads such as PCAP file reading)
Number of processed packets:    18
Total data volume:              0.0017 MB
Feature extraction throughput:  6.2498 MB/s (0.0488 Gbps)
Core processing throughput:     6.2498 MB/s (0.0488 Gbps)
E2E throughput:                 0.9222 MB/s (0.0072 Gbps)
========================================================


Filtering statistics: 0 samples retained and 0 samples filtered out
Inference engine destroyed
```

The input is `icmp_data.pcap`. According to the performance statistics, 18 packets are processed. The filtering statistics show that 0 samples are retained and 0 samples are filtered out, indicating that no flow data is extracted during feature extraction and all ICMP packets, which do not meet the requirements, are discarded.
