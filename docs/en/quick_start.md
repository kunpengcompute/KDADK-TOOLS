# Quick Start

KDADK-TOOLS provides a demo example. You can use the executable file `kdadk_demo` to demonstrate the feature extraction, model training, model verification, and online inference processes. For details, see [User Guide](user_guide.md).

## Feature Extraction

The `-f` parameter indicates the feature extraction mode. The input is the collected traffic data in PCAP format, and the output is the feature data in CSV format. The input data `input.pcap` can be collected by using the [traffic collection tools](user_guide.md#traffic-collection-tools).

```bash
/opt/KDADK-TOOLS/demo/build/kdadk_demo -f input.pcap -c output.csv
```

The command output is as follows.

```bash
========== Feature extraction mode ==========
Input file: input.pcap
Output file: output.csv
Output rawbow: No
File writer created: output.csv (format=CSV, mode=features_only)
Processing completed:
  - Total number of packets: 2728091
  - Total number of flows: 14039
  - Output file: output.csv
File writer destroyed
```

- Filter data in the CSV file after feature extraction.
  - Filter out flows with less than 16 packets (feature row).
  - Delete the `flow_id`, `src_ip`, and `dst_ip` columns.
You can use the `feature_filter.py` script in the `data` directory to quickly process data. You need to specify the configuration file path and data folder path (directory where the CSV file generated after feature extraction is located).

    ```bash
    python /opt/KDADK-TOOLS/data/feature_filter.py /opt/KDADK-TOOLS/src/config.yaml /path/to/csv_data_folder
    ```

## Model Training

The `-t` parameter indicates the model training mode. Configure the path to the CSV file required for model training in `src/config.yaml`. For details, see [Model Training](user_guide.md#model-training--t) in the user guide.

```bash
/opt/KDADK-TOOLS/demo/build/kdadk_demo -t /opt/KDADK-TOOLS/src/config.yaml
```

## Model Verification

The `-e` parameter indicates the model verification mode. Configure the path to the CSV file required for model verification in `src/config.yaml`. For details, see [Model Verification](user_guide.md#model-verification--e) in the user guide.

```bash
/opt/KDADK-TOOLS/demo/build/kdadk_demo -e /opt/KDADK-TOOLS/src/config.yaml
```

## Online Inference

The `-r` parameter indicates the online inference mode. Configure the path to the trained model weights in `src/config.yaml`. Ensure that the filtering criteria are the same as those used during model training. For details, see [Online Inference](user_guide.md#online-inference--r) in the user guide.

The online inference mode supports two types of data input: offline PCAP data and real-time packet capture data on a network interface, specified by the `-p` and `-i` parameters, respectively.

Method 1: The input is multiple PCAP files, the output is a CSV file containing the features and prediction results, and the `-p` parameter is used to specify the path to the PCAP files.

```bash
/opt/KDADK-TOOLS/demo/build/kdadk_demo -r /opt/KDADK-TOOLS/src/config.yaml -p file1.pcap -p file2.pcap -m 1 -c result.csv
```

Method 2: The input is real-time packet capture data on a network interface, the output is a CSV file containing the features and prediction results, and the `-i` parameter is used to specify the network interface name.

```bash
/opt/KDADK-TOOLS/demo/build/kdadk_demo -r /opt/KDADK-TOOLS/src/config.yaml -i eth0 -c result.csv
```

You can run the following command to query the network interface information:

```bash
/opt/KDADK-TOOLS/demo/build/kdadk_demo -l
```
