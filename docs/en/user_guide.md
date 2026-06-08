# User Guide

This user guide describes how to use the `kdadk_demo` program, and the traffic collection and labeling tools.

## Example Usage

You can use the `-h` parameter to view the functions of the `kdadk_demo` program as follows:

```bash
/opt/KDADK-TOOLS/demo/build/kdadk_demo -h
Usage: /opt/KDADK-TOOLS/demo/build/kdadk_demo [options]

Options:
  -t <config.yaml>        Model training mode
  -e <config.yaml>        Model verification mode
  -f <file.pcap>          Feature extraction mode (used together with -c or -j)
  -r <config.yaml>        Online inference mode (used together with -c or -j)
  -p <file.pcap>          Input PCAP files (one or more)
  -i <interface>          Real-time packet capture on a network interface
  -m <mode>               Inference mode (0: single-flow; 1 (default): batch processing; 2: file)
  -c <output.csv>         Path to the output CSV file (mutually exclusive with -j)
  -j <output.json>        Path to the output JSON file (mutually exclusive with -c)
  -w                      rawbow contained in the output (only used for -f)
  -l                      Listing the available network interfaces of the current server
  -h                      Displaying the help information

Note:
  Either -c or -j can be selected to specify the output file format in CSV or JSON, respectively.
  The file specified by -c must end with .csv.
  The file specified by -j must end with .json.
  Outputting rawbow (specified by -w) is supported only in the feature extraction mode (specified by -f). The rawbow feature column information is used only for manual verification after traffic labeling.

Examples:
  # Feature extraction - CSV format
  /opt/KDADK-TOOLS/demo/build/kdadk_demo -f input.pcap -c output.csv

  # Feature extraction - JSON format with rawbow
  /opt/KDADK-TOOLS/demo/build/kdadk_demo -f input.pcap -j output.json -w

  # Online inference - multiple PCAP files, CSV file output
  /opt/KDADK-TOOLS/demo/build/kdadk_demo -r config.yaml -p file1.pcap -p file2.pcap -m 1 -c result.csv

  # Online inference - real-time packet capture on a network interface, JSON output
  /opt/KDADK-TOOLS/demo/build/kdadk_demo -r config.yaml -i eth0 -j result.json

  # Listing available network interfaces
  /opt/KDADK-TOOLS/demo/build/kdadk_demo -l
```

The core functional modules include model training (`-t`), model verification (`-e`), feature extraction (`-f`), and online inference (`-r`).

### 1. Model Training (`-t`)

- Parameter: `-t config.yaml`
- Function: Reads the [`config`](#7-config-configuration-file) file, calls the model training script (Python), and executes the model training task.
- `config` parameters: `training_data_paths`, `model_path_pkl`, `scaler_path_pkl`, `model_path_onnx`, and `scaler_path_json`

### 2. Model Verification (`-e`)

- Parameter: `-e config.yaml`
- Function: Reads the [`config`](#7-config-configuration-file) file, calls the model verification script (Python), and performs model verification.
- `config` parameters: `evaluation_data_paths`, `model_path_pkl`, `scaler_path_pkl`, `output_dir`, `classification_report_file`, and `predictions_detail_file`

### 3. Feature Extraction (`-f`)

- Parameter: `-f xxx.pcap -c output.csv` or `-f xxx.pcap -j output.json`
- Function: Inputs the PCAP offline network traffic for protocol parsing and feature extraction.
- Output: The feature extraction result is saved to a file by using `-c output.csv` or `-j output.json`.
- Optional parameter: Specify `-w` to output `rawbow`. The `rawbow` feature column information is used only for manual verification after traffic labeling.
- Filter data in the CSV file after feature extraction.
  - Filter out flows with less than 16 packets (feature row).
  - Delete the `flow_id`, `src_ip`, and `dst_ip` columns.
You can use the `feature_filter.py` script in the `data` directory to quickly process data. You need to specify the `config.yaml` file path and data folder path. Read the filtering conditions in the `config` file: `columns_to_remove` and `filter_packets`. If the `rawbow` information is output during feature extraction, data filtering for the `rawbow` feature column is needed (by adding `354` to `columns_to_remove`). The `rawbow` feature column cannot be input for model training and verification. In the online inference mode, the `rawbow` feature column is filtered out by default.

    ```bash
    python /opt/KDADK-TOOLS/data/feature_filter.py /opt/KDADK-TOOLS/src/config.yaml /path/to/csv_data_folder
    ```

### 4. Online Inference (`-r`)

- Parameter: `-r config.yaml`
- Function: Reads the [`config`](#7-config-configuration-file) file, calls the inference module, and executes the online inference task, including feature extraction, data filtering, and feature inference.
- `config` parameters: `model_path_onnx`, `scaler_path_json`, `onnx_log_level`, `columns_to_remove`, and `filter_packets`
- Input data sources:
  - `-p xxx.pcap`:
      - The data is read from the PCAP file. You can specify multiple PCAP files as the input by using `-p` multiple times.
      - `-m 0`, `-m 1`, or `-m 2` is used to specify the inference mode. `0` indicates the single-flow mode, `1` indicates the batch processing mode, and `2` indicates the file mode. By default, the batch processing mode is used.
      - The classification report and result are output. To output the classification result, you need to specify a file path. The classification report is saved in the default path.
  - `-i interface`: The traffic on a network interface is captured in real time for feature extraction and inference. Outputting the classification result is optional. You can use `-l` to query network interfaces.
- Output: The classification result is output to a file by using either `-c output.csv` or `-j output.json`.

### 5. Checking Network Interfaces (`-l`)

- Parameter: `-l`
- Function: Lists the network interface information of the current server.

### 6. Help Information (`-h`)

- Parameter: `-h`
- Function: Displays the help information about parameter usage.

### 7. `config` Configuration File
>
>❗**NOTICE:**
> All paths can be absolute or relative paths. **Absolute paths are recommended**. When a relative path is used, the path starts from the root directory of the current repository by default.

- `training_data_paths`: input data for **model training**. Each type of data is stored in an array. Different arrays represent different types of data. A type of data can be stored in multiple CSV files.

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

- `model_path_pkl`: path to the trained model weight, which is also the input path for model evaluation (Python).

```bash
model_path_pkl: result/model_classifier_2.pkl
```

- `scaler_path_pkl`: path to the trained scaler weight, which is also the input path for model evaluation (Python).

```bash
scaler_path_pkl: result/scaler_2.pkl
```

- `model_path_onnx`: path to the trained model weight, which is also the input path for model evaluation (C++).

```bash
model_path_onnx: result/model_classifier_2.onnx
```

- `scaler_path_json`: path to the trained scaler weight, which is also the input path for model evaluation (C++).

```bash
scaler_path_json: result/scaler_2.json
```

- `evaluation_data_paths`: input data for **model evaluation**. Each type of data is stored in an array. Different arrays represent different types of data. A type of data can be stored in multiple CSV files.

```bash
evaluation_data_paths:
  - ['data/bilibili/csv/bilibili_20250721_141827_21600_android_20.csv']
  - ['data/wenxiaoyan/csv/wenxiaoyan_20250804_205038_144000_android_20.csv']
```

- `output_dir`: output folder of the model evaluation result.

```bash
output_dir: result
```

- `classification_report_file`: path to the flow classification report.

```bash
classification_report_file: result/classification_report_2.txt
```

- `predictions_detail_file`: path to the detailed flow classification result.

```bash
predictions_detail_file: result/predictions_detail_2.csv
```

- `onnx_log_level`: ONNX Runtime configuration.

```bash
onnx_log_level: WARNING
# VERBOSE   Detailed information
# INFO      General information
# WARNING   Warning
# ERROR     Error
# FATAL     Fatal error
```

- `columns_to_remove`: filters the feature extraction result before feature inference. The number in the list indicates the subscript of the feature column minus 1. The default values are `0`, `1`, and `2`, indicating `flow_id`, `src_ip`, and `dst_ip`, respectively. You can adjust the values as required, but ensure that the values are the same during model training and inference.

```bash
columns_to_remove: [0, 1, 2]
```

- `filter_packets`: filters the feature extraction result before feature inference. Specifically, the sum of `send_packet_nums` and `receive_packet_nums` must be smaller than `filter_packets`. The default value of `filter_packets` is `16`. You can adjust the value as required, but ensure that the value is the same during model training and inference.

```bash
filter_packets: 16
```

## Tool Usage

This repository provides traffic collection tools and a traffic labeling tool, which are stored in `KDADK-TOOLS/tools/`. The following describes how to use them.

### 1. Traffic Collection Tools

The traffic collection tools are stored in the `tools/capture/` directory and contains the following files:

- `capture_win.ps1` is used to capture packets using Wireshark in the Windows environment. Ensure that Wireshark has been installed before using this tool. For details about how to use this tool, run the `./capture_win.ps1 -h` command.
- `capture_linux.sh` is used to capture packets using tcpdump in the Linux environment. Ensure that tcpdump has been installed before using this tool. For details about how to use this tool, run the `sh capture_linux.sh -h` command.
- `capture_linux_docker.sh` is used to capture packets using tcpdump in the Docker container in the Linux environment. Ensure that the Docker environment is correct. For details about how to use this tool, run the `sh capture_linux_docker.sh -h` command.

The following uses `capture_linux.sh` as an example.

```bash
sh capture_linux.sh -d 60 -n input -i eth0
# -d 60  The packet capture duration is 60 seconds.
# -n input  The output file name is input.pcap. For details about the path, see the output information on the terminal.
# -i eth0  The traffic on the eth0 network interface is captured. You can run the -l command to view the available network interfaces on the current server (up indicates that a network interface is available).
```

### 2. Traffic Labeling Tool

The traffic labeling tool is stored in the `tools/labeling/` directory where contains the following files:

- `labeling.py`: labeling tool. For details about how to use it, see the following description.

```bash
python labeling.py -h

usage: labeling.py [-h] [--mode {train,inference}] [--config CONFIG]

Labeling tool

options:
  -h, --help            show this help message and exit
  --mode {train,inference}
                        Running mode: training or inference
  --config CONFIG       Path to the configuration file (default: config_label.yaml)

Examples:
  python labeling.py --mode train                    # Use the default configuration file for training.
  python labeling.py --mode inference                # Use the default configuration file for inference. By default, a CSV file in each folder is used.
  python labeling.py --mode train --config ./config_label.yaml  # Specify the configuration file.
```

- `config_label.yaml`: configuration file, which can be used to configure the parameters of the labeling tool.

>❗**NOTICE:**
> During inference, only one CSV feature file can be input. You can set `inference=False` in the inference source code as required to cancel this restriction.

```bash
# Training configuration for the traffic labeling tool
train:
# Data involved in training. Each path can contain multiple feature extraction CSV files. The files support the dictionary mode and each can be configured with a path and name.
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

# Name of the current training model weight
  model_name: "labeling_train"

# Number of clusters trained by the current dataset. The number of clusters must be greater than 1. 
  n_clusters: 3

# 30% of the dataset is used for testing, and 70% is used for training. The value range is (0, 1).
  test_size: 0.3

# Location for saving the output results. The output files contain the model weight and scaler weight.
  results_dir: results        

# Inference configuration for the traffic labeling tool
inference:

# Data involved in inference. Each path can contain multiple feature extraction CSV files. The files support the dictionary mode and each can be configured with a path and name.
  data_paths: [
    'data/youku/csv',
    'data/yuanbao/csv',
    'data/wymusic/csv'
  ]

# Path for loading the pre-trained model weight
  model_path: "results/labeling_train_model.pkl"

# Path for loading the pre-trained scaler weight 
  scaler_path: "results/labeling_train_scaler.pkl"

# Path for saving the output result
  output_file: "output/output.csv"  

```

- `results`: path to the tool output result, which can be modified in the configuration file.
- `output`: folder of the output clustering prediction result, which can be modified in the configuration file.
