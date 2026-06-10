# 安装指南

KDADK-TOOLS提供了网络流分类参考示例的依赖包安装及编译方法。

## 环境要求

KDADK-TOOLS当前适配的处理器和操作系统为鲲鹏920新型号处理器、openEuler操作系统，若您在使用过程中遇到问题，请先检查使用的环境是否在已验证的环境范围内。

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
| KTAG | 1.0.0 |

## 获取项目代码

克隆KDADK-TOOLS代码仓库到`/opt`目录下（这里以`/opt`目录为例，后续操作均在`/opt`目录下进行，或者可以选择其他自定义目录）：

```bash
cd /opt
git clone https://gitcode.com/boostkit/KDADK-TOOLS.git
cd KDADK-TOOLS
```

## 依赖准备

### 1. 安装yaml-cpp

下载源码包[yaml-cpp](https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz)。

将下载好的源码放在`/opt/KDADK-TOOLS/util/`文件夹下，解压压缩包，然后进到`yaml-cpp-0.8.0`文件夹下，进行编译安装。

```bash
tar -xzf yaml-cpp-0.8.0.tar.gz
cd yaml-cpp-0.8.0
mkdir -p build
cd build
cmake .. -DYAML_BUILD_SHARED_LIBS=ON
make -j32
make install
```

出现下述回显即为成功：

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

### 2. 安装onnxruntime

下载源码包[onnxruntime](https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-aarch64-1.22.0.tgz)。

将下载好的源码放在`/opt/KDADK-TOOLS/util/`文件夹下，解压压缩包，然后设置环境变量。

```bash
tar -xzf onnxruntime-linux-aarch64-1.22.0.tgz
export ONNXRUNTIME_HOME=/opt/KDADK-TOOLS/util/onnxruntime-linux-aarch64-1.22.0
export LD_LIBRARY_PATH=$ONNXRUNTIME_HOME/lib:$LD_LIBRARY_PATH
```

### 3. 配置python环境

在`/opt/KDADK-TOOLS/src/py/packages.txt`文件中记录了离线训练需要的python环境依赖，可以执行下述命令安装python环境依赖。

```bash
pip install -r /opt/KDADK-TOOLS/src/py/packages.txt
```

### 4. 配置特征提取和推理库

下载[KTAG](https://gitcode.com/boostkit/KDADK-TOOLS/releases/download/v1.0.0/KDADK-1.0.0.zip
)动态库，执行下述命令进行配置。

```bash
cd /opt/KDADK-TOOLS/
mkdir lib
cd lib
wget https://gitcode.com/boostkit/KDADK-TOOLS/releases/download/v1.0.0/KDADK-1.0.0.zip
unzip KDADK-1.0.0.zip
```

## 编译指导

进入到`/opt/KDADK-TOOLS/demo`目录下执行下述命令编译。

```bash
cd /opt/KDADK-TOOLS/demo/
mkdir build
cd build
cmake ..
make
```

编译得到可执行的二进制程序：`kdadk_demo`。
