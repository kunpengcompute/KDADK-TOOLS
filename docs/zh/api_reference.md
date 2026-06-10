
# API参考

## 特征提取接口

### extractor_init

**功能**

创建并初始化特征提取器。

**函数接口**

```c
feature_extractor *extractor_init(int link_type);
```

**参数**

* `link_type`： 链路层类型，支持`DLT_EN10MB(1)`、`DLT_LINUX_SLL(113)`。

**返回值**

* 如果调用接口成功则返回特征提取器句柄，如果执行失败则返回NULL。

### extractor_destroy

**功能**

销毁特征提取器。

**函数接口**

```c
void extractor_destroy(feature_extractor *extractor);
```

**参数**

* `extractor`：  特征提取器句柄。

**返回值**

* 无

### process_packet

**功能**

处理单个数据包，进行packet的协议解析及单个流的特征提取。

**函数接口**

```c
int process_packet(feature_extractor *extractor, const unsigned char *packet, uint32_t packet_len,
                   const struct timeval *ts, feature_vector *features, int *has_feature);
```

**参数**

* `extractor`：  特征提取器句柄。
* `packet`：  数据包指针。
* `packet_len`：  数据包长度。
* `ts`： 时间戳。
* `features`：  输出特征向量（单个流）。
* `has_feature`： 输出标志，表示是否有特征提取（流结束才会进行特征提取）。

**返回值**

* 状态码：EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败。

### process_packet_with_rawbow

**功能**

处理单个数据包（提取特征向量和rawbow），进行packet的协议解析及单个流的特征提取，包含rawbow。

**函数接口**

```c
int process_packet_with_rawbow(feature_extractor *extractor, const unsigned char *packet, uint32_t packet_len,
                               const struct timeval *ts, feature_vector *features, rawbow *rawbows, int *has_feature);
```

**参数**

* `extractor`：  特征提取器句柄。
* `packet`：  数据包指针。
* `packet_len`：  数据包长度。
* `ts`： 时间戳。
* `features`：  输出特征向量（单个流）。
* `rawbows`： 输出rawbow（单个流）。
* `has_feature`： 输出标志，表示是否有特征提取（流结束才会进行特征提取）。

**返回值**

* 状态码：EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败。

### extractor_finalize

**功能**

处理所有剩余的流（仅提取特征向量）。

**函数接口**

```c
int extractor_finalize(feature_extractor *extractor, feature_vector_list *features);
```

**参数**

* `extractor`：  特征提取器句柄。
* `features`：   输出特征向量列表。

**返回值**

* 状态码：EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败。

### extractor_finalize_with_rawbow

**功能**

处理所有剩余的流（提取特征向量和rawbow）。

**函数接口**

```c
int extractor_finalize_with_rawbow(feature_extractor *extractor, feature_vector_list *features, rawbow_list *rawbows);
```

**参数**

* `extractor`：  特征提取器句柄。
* `features`：   输出特征向量列表。
* `rawbows`： 输出rawbow列表。

**返回值**

* 状态码：EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败。

### extractor_get_statistics

**功能**

获取统计信息。

**函数接口**

```c
int extractor_get_statistics(feature_extractor *extractor, statistics *stats);
```

**参数**

* `extractor`：  特征提取器句柄。
* `stats`：   输出统计信息。  

**返回值**

* 状态码：EXTRACTOR_SUCCESS表示成功，EXTRACTOR_ERROR表示失败。

### extractor_reset_statistics

**功能**

重置统计信息。

**函数接口**

```c
void extractor_reset_statistics(feature_extractor *extractor);
```

**参数**

* `extractor`：  特征提取器句柄。

**返回值**

* 无

## 特征推理接口

### inference_init

**功能**

创建并初始化推理引擎。

**函数接口**

```c
inference_engine *inference_init(const inference_config *config);
```

**参数**

* `config`：推理配置（包含配置文件路径）。

**返回值**

* 如果调用接口成功则返回推理引擎句柄，如果执行失败则返回NULL。

### inference_destroy

**功能**

销毁推理引擎。

**函数接口**

```c
void inference_destroy(inference_engine *engine);
```

**参数**

* `engine`：推理引擎句柄。

**返回值**

* 无

### inference_predict

**功能**

批量推理。

**函数接口**

```c
int inference_predict(inference_engine *engine, const feature_vector_list *features, inference_result *result);
```

**参数**

* `engine`：推理引擎句柄。
* `features`： 特征向量列表。
* `result`：输出推理结果。

**返回值**

* 状态码：INFERENCE_SUCCESS表示成功，INFERENCE_ERROR表示失败。
