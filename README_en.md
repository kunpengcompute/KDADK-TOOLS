# KDADK-TOOLS

## Latest Updates

[2026-06-30]: KDADK-APPID has been renamed KTAG.
[2026-03-30]: Released KDADK-TOOLS 1.0.0, updated the header files and software packages of the feature extraction and inference APIs, and provided a reference example for network flow classification of Kunpeng BoostKit for Intelligent Data Analytics (IDA).

## Project Introduction

KTAG is the network flow classification library of Kunpeng BoostKit for IDA. It includes flow aggregation, protocol detection, feature calculation, an AI engine, and a training tool package. It is optimized for Kunpeng features and can analyze both plaintext and encrypted traffic in real time. This addresses the limitations of conventional rule-based data distribution solutions that cannot analyze encrypted traffic, providing an end-to-end reference solution for the industry's common challenges.
KDADK-TOOLS is a demo implemented based on the APIs of the network flow classification library. It provides examples of using feature extraction and inference APIs, and tools for traffic collection, feature filtering, model training, and adaptive labeling.

## Directory Structure

The project directory structure is as follows:

```text
|—— data                                                      # Script tool for data processing after feature extraction
│   ├── feature_filter.py                                     # Feature filtering script
|—— demo                                                      # Source code of the demo implemented based on the APIs of the network flow classification library
|—— docs                                                      # Documentation directory
│   └── en                                                    # English document directory
│       ├── quick_start.md                                    # Quick Start
│       ├── release_notes.md                                  # Release Notes
│       ├── installation_guide.md                             # Installation Guide
│       ├── user_guide.md                                     # User Guide
│       ├── api_reference.md                                  # API Reference
│       ├── faq.md                                            # FAQs
|—— include                                                   # Header files of the feature extraction and inference APIs
│   ├── kdadk_feature_extract.h                               # Header file of the feature extraction APIs
│   ├── kdadk_inference.h                                     # Header file of the feature inference APIs
|—— src                                                       # Model training and verification tools, and configuration file config.yaml
│   └── py                                                    # Directory for storing Python scripts for model training and verification
│   ├── config.yaml                                           # Configuration file
|—— tools                                                     # Traffic collection and labeling tools
│   ├── capture/                                              # Directory for storing the traffic collection tools
│   ├── labeling/                                             # Directory for storing the traffic labeling tool
|—— util                                                      # Installation directory of dependency packages required for environment deployment
|—— README_en.md                                                 # Introduction to the KDADK-TOOLS project
```

## Release Notes

For details about feature changes in each version, see [Release Notes](docs/en/release_notes.md).


## Environment Deployment

KDADK-TOOLS can function properly on new Kunpeng 920 processor models running openEuler. If you encounter any problem with KDADK-TOOLS, confirm that your environment is a verified environment.

**Table 1** Verified environments for KDADK-TOOLS

| OS | CPU |
| ------------ | ------------ |
| openEuler 22.03 LTS SP4  | New Kunpeng 920 processor model |
| openEuler 24.03 LTS SP2  | New Kunpeng 920 processor model |

**Table 2** Software requirements

| Software Name| Version|
| --- | --- |
| GCC | 10.3.1 |
| CMake | 3.22.0 |
| yaml-cpp | 0.8.0 |
| ONNX Runtime| 1.22.0 |
| Python | 3.9.9 or later|
| KTAG | 1.0.0 |

For details about the installation method, see [Installation Guide](docs/en/installation_guide.md).

After the compilation, the executable binary program `kdadk_demo` is obtained.

## Quick Start

The quick start of KDADK-TOOLS is implemented by using `kdadk_demo`. For details, see [Quick Start](docs/en/quick_start.md).

## Related Documents

This feature provides the following learning documents for reference.

| Document| Description|
| -- | -- |
| [Quick Start](docs/en/quick_start.md)| Provides the quick usage of the KDADK-TOOLS demo.|
| [Release Notes](docs/en/release_notes.md)| Provides version updates.|
| [Installation Guide](docs/en/installation_guide.md)| Provides the methods of installing dependencies and compiling the KDADK-TOOLS demo.|
| [User Guide](docs/en/user_guide.md)| Provides detailed usage of the KDADK-TOOLS demo, traffic collection tools, and the traffic labeling tool.|
| [API Reference](docs/en/api_reference.md)| Provides the description of the feature extraction and inference APIs.|
| [FAQs](docs/en/faq.md)| Lists some frequently asked questions.|

## Contribution Statement

If you have any questions or want to provide feedback on feature requirements and bug reports, you can submit issues. For details, see the [contribution guideline](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md).

## Disclaimer

To KDADK-TOOLS users:

- This project provides only reference examples and tools. You are responsible for any risks and should carefully review the following information:
    - Data processing and deletion: Users are responsible for managing and deleting any data generated while using this tool. Users are advised to delete such data promptly after use to prevent information leakage.
    - Data confidentiality and transmission: Users understand and agree not to share or transmit any data generated by this tool. Neither the tool nor its developers are responsible for any information leaks, data breaches, or other negative consequences.
    - User input security: Users are responsible for the security of any commands they enter and for any risks or losses resulting from improper input. The tool and its developers are not liable for issues caused by incorrect command usage.
- Disclaimer scope: This disclaimer applies to all individuals and entities using this tool. It is not recommended that this tool be used for any commercial purposes. By using the tool, you acknowledge and accept this statement and assume all risks and responsibilities arising from its use. If you do not agree, please stop using the tool immediately.
- Before using this tool, please read and understand the preceding disclaimer. If you have any questions, contact the developer.

## License

This project is licensed under BSD 3-Clause License. For details, see [LICENSE](LICENSE).

The documents of this project are licensed under CC-BY 4.0. For details, see [LICENSE](docs/LICENSE).
