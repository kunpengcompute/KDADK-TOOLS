# 安装指南
## 1. 依赖准备
在根目录下新建util目录，下面的依赖都放置在util目录下
```
mkdir util
cd util
```

### 1.1 安装yaml-cpp
[这里](https://github.com/jbeder/yaml-cpp/releases/tag/0.8.0)下载源码包

将下载好的源码放在`KDADK-TOOLS/util/`文件夹下，进到`KDADK-TOOLS/util/yaml-cpp`

```
# 进到yaml-cpp目录下
cd yaml-cpp

# 创建build文件夹
mkdir -p build

# 进入build
cd build

# cmake构建
cmake .. -DYAML_BUILD_SHARED_LIBS=ON

# make编译
make -j32

# install
make install
```
出现下述回显即为成功：

```
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

### 1.2 安装onnxruntime
[这里](https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-aarch64-1.22.0.tgz)下载源码包

将下载好的源码放在`KDADK-TOOLS/util/`文件夹下

```
# 解压
tar -xzf onnxruntime-linux-aarch64-1.22.0.tgz

# 设置环境变量
export ONNXRUNTIME_HOME=/path/to/your/onnxruntime
export LD_LIBRARY_PATH=$ONNXRUNTIME_HOME/lib:$LD_LIBRARY_PATH
```
### 1.3 配置python环境

在`src/py/packages.txt`文件中记录了离线训练需要的python环境依赖，可以执行下述命令安装python环境依赖
```
# 在根目录下执行
pip install -r src/py/packages.txt
```
### 1.4 准备特征提取和推理库

将下载好的动态库放到`KDADK-TOOLS/lib/`文件夹下

```
[root@master KDADK-TOOLS]# mkdir lib
[root@master KDADK-TOOLS]# cd lib
[root@master lib]# ls
libKDADK_feature_extract.so  libKDADK_inference.so
```
## 2. 编译指导
进入到KDADK-TOOLS/demo执行下述命令

```
mkdir build
cd build
cmake ..
make
```