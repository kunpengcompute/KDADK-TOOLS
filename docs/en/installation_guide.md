# Installation Guide

KDADK-TOOLS provides the methods of installing dependencies and compiling the reference example of network flow classification.

## Obtaining Project Code

Clone the KDADK-TOOLS code repository to the `/opt` directory. (The `/opt` directory is used as an example for all subsequent operations. You can also select another user-defined directory.)

```bash
# Go to the /opt directory. You can change it to a user-defined directory.
cd /opt

# Clone the KDADK-TOOLS code repository.
git clone https://gitcode.com/boostkit/KDADK-TOOLS.git

# Go to the KDADK-TOOLS directory.
cd KDADK-TOOLS
```

## Preparing Dependencies

### 1. Installing yaml-cpp

Download the [yaml-cpp source package](https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz).

Save the downloaded source code to the `KDADK-TOOLS/util/` folder and go to the `KDADK-TOOLS/util/yaml-cpp` directory.

```bash
# Go to the yaml-cpp directory.
cd yaml-cpp

# Create a build folder.
mkdir -p build

# Go to build.
cd build

# Use cmake for building.
cmake .. -DYAML_BUILD_SHARED_LIBS=ON

# Use make for compilation.
make -j32

# install
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

### 2. Installing ONNX Runtime

Download the [ONNX Runtime source package](https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-aarch64-1.22.0.tgz).

Save the downloaded source code to the `KDADK-TOOLS/util/` folder.

```bash
# Decompress the package.
tar -xzf onnxruntime-linux-aarch64-1.22.0.tgz

# Set environment variables.
export ONNXRUNTIME_HOME=/opt/KDADK-TOOLS/util/onnxruntime-linux-aarch64-1.22.0
export LD_LIBRARY_PATH=$ONNXRUNTIME_HOME/lib:$LD_LIBRARY_PATH
```

### 3. Configuring the Python environment

The `src/py/packages.txt` file records the Python environment dependencies required for offline training. You can run the following command to install the Python environment dependencies:

```bash
# Run the following command in /opt/KDADK-TOOLS/:
pip install -r /opt/KDADK-TOOLS/src/py/packages.txt
```

### 4. Preparing feature extraction and inference libraries

Downloaded [KDADK-APPID](https://gitcode.com/boostkit/KDADK-TOOLS/releases/download/v1.0.0/KDADK-1.0.0.zip).

```bash
# Run the following command in /opt/KDADK-TOOLS/:
mkdir lib
cd lib
wget https://gitcode.com/boostkit/KDADK-TOOLS/releases/download/v1.0.0/KDADK-1.0.0.zip
unzip KDADK-1.0.0.zip
cd ..
```

## Compilation Guide

Go to the `KDADK-TOOLS/demo` directory and run the following commands to perform compilation:

```bash
# Go to the KDADK-TOOLS/demo directory.
cd demo

# Create a build folder.
mkdir build

# Go to the build directory.
cd build

# Use cmake for building.
cmake ..

# Use make for compilation.
make
```

After the compilation, the executable binary program `kdadk_demo` is obtained.
