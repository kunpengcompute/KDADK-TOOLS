# Installation Guide

KDADK-TOOLS provides the methods of installing dependencies and compiling the reference example of network flow classification.

## Environment Requirements

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

## Obtaining Project Code

Clone the KDADK-TOOLS code repository to the `/opt` directory. (The `/opt` directory is used as an example for all subsequent operations. You can also select another user-defined directory.)

```bash
cd /opt
git clone https://gitcode.com/boostkit/KDADK-TOOLS.git
cd KDADK-TOOLS
```

## Preparing Dependencies

1. Install yaml-cpp.

    Download the [yaml-cpp source package](https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz) to the `/opt/KDADK-TOOLS/util/` folder, decompress the package, go to the `yaml-cpp-0.8.0` folder, and compile and install the source code.

    ```bash
    tar -xzf yaml-cpp-0.8.0.tar.gz
    cd yaml-cpp-0.8.0
    mkdir -p build
    cd build
    cmake .. -DYAML_BUILD_SHARED_LIBS=ON
    make -j32
    make install
    ```

    If the following information is displayed, the operation is successful:

    ```bash
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

2. Install ONNX Runtime.

    Download the [ONNX Runtime source package](https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-aarch64-1.22.0.tgz) to the `/opt/KDADK-TOOLS/util/` folder, decompress the package, and set environment variables.

    ```bash
    tar -xzf onnxruntime-linux-aarch64-1.22.0.tgz
    export ONNXRUNTIME_HOME=/opt/KDADK-TOOLS/util/onnxruntime-linux-aarch64-1.22.0
    export LD_LIBRARY_PATH=$ONNXRUNTIME_HOME/lib:$LD_LIBRARY_PATH
    ```

3. Configure the Python environment.

    The `/opt/KDADK-TOOLS/src/py/packages.txt` file records the Python environment dependencies required for offline training. You can run the following command to install the Python environment dependencies:

    ```bash
    pip install -r /opt/KDADK-TOOLS/src/py/packages.txt
    ```

4. Configure feature extraction and inference libraries.

    Download the [KTAG](https://gitcode.com/boostkit/KDADK-TOOLS/releases/download/v1.0.0/KDADK-1.0.0.zip) dynamic library, and run the following commands to configure it:

    ```bash
    cd /opt/KDADK-TOOLS/
    mkdir lib
    cd lib
    wget https://gitcode.com/boostkit/KDADK-TOOLS/releases/download/v1.0.0/KDADK-1.0.0.zip
    unzip KDADK-1.0.0.zip
    ```

## Compiling Code

Go to the `/opt/KDADK-TOOLS/demo` directory and run the following commands to perform compilation:

```bash
cd /opt/KDADK-TOOLS/demo/
mkdir build
cd build
cmake ..
make
```

After the compilation, the executable binary program `kdadk_demo` is obtained.
