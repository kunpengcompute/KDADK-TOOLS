
# API Reference

## 1. Feature Extraction APIs

### extractor_init

**Function**

Creates and initializes a feature extractor.

**Function API**

````bash
feature_extractor *extractor_init(int link_type);
````

**Parameter**

* `link_type`: link layer type, which can be `DLT_EN10MB(1)` or `DLT_LINUX_SLL(113)`.

**Return Values**

* A successful API call returns the handle of the feature extractor. A failed API call returns `NULL`.

### extractor_destroy

**Function**

Destroys a feature extractor.

**Function API**

````bash
void extractor_destroy(feature_extractor *extractor);
````

**Parameter**

* `extractor`: feature extractor handle.

**Return Values**

* None

### process_packet

**Function**

Processes a single packet by parsing the packet protocol and extracting features of a single flow.

**Function API**

````bash
int process_packet(feature_extractor *extractor, const unsigned char *packet, uint32_t packet_len,
                   const struct timeval *ts, feature_vector *features, int *has_feature);
````

**Parameters**

* `extractor`: feature extractor handle.
* `packet`: data packet pointer.
* `packet_len`: data packet length.
* `ts`: timestamp.
* `features`: output feature vector (single flow).
* `has_feature`: output flag, indicating whether feature extraction is performed. Feature extraction is performed only when a flow ends.

**Return Values**

* Status code: `EXTRACTOR_SUCCESS` indicates that the operation is successful, and `EXTRACTOR_ERROR` indicates that the operation fails.

### process_packet_with_rawbow

**Function**

Processes a single data packet by parsing the packet protocol and extracting features of a single flow, including `rawbow`.

**Function API**

````bash
int process_packet_with_rawbow(feature_extractor *extractor, const unsigned char *packet, uint32_t packet_len,
                               const struct timeval *ts, feature_vector *features, rawbow *rawbows, int *has_feature);
````

**Parameters**

* `extractor`: feature extractor handle.
* `packet`: data packet pointer.
* `packet_len`: data packet length.
* `ts`: timestamp.
* `features`: output feature vector (single flow).
* `rawbows`: output `rawbow` (single flow).
* `has_feature`: output flag, indicating whether feature extraction is performed. Feature extraction is performed only when a flow ends.

**Return Values**

* Status code: `EXTRACTOR_SUCCESS` indicates that the operation is successful, and `EXTRACTOR_ERROR` indicates that the operation fails.

### extractor_finalize

**Function**

Processes all remaining flows (only feature vectors are extracted).

**Function API**

````bash
int extractor_finalize(feature_extractor *extractor, feature_vector_list *features);
````

**Parameters**

* `extractor`: feature extractor handle.
* `features`: output feature vector list.

**Return Values**

* Status code: `EXTRACTOR_SUCCESS` indicates that the operation is successful, and `EXTRACTOR_ERROR` indicates that the operation fails.

### extractor_finalize_with_rawbow

**Function**

Processes all remaining flows (feature vectors and `rawbow` are extracted).

**Function API**

````bash
int extractor_finalize_with_rawbow(feature_extractor *extractor, feature_vector_list *features, rawbow_list *rawbows);
````

**Parameters**

* `extractor`: feature extractor handle.
* `features`: output feature vector list.
* `rawbows`: output `rawbow` list.

**Return Values**

* Status code: `EXTRACTOR_SUCCESS` indicates that the operation is successful, and `EXTRACTOR_ERROR` indicates that the operation fails.

### extractor_get_statistics

**Function**

Obtains statistics.

**Function API**

````bash
int extractor_get_statistics(feature_extractor *extractor, statistics *stats);
````

**Parameters**

* `extractor`: feature extractor handle.
* `stats`: output statistics. 

**Return Values**

* Status code: `EXTRACTOR_SUCCESS` indicates that the operation is successful, and `EXTRACTOR_ERROR` indicates that the operation fails.

### extractor_reset_statistics

**Function**

Resets statistics.

**Function API**

````bash
void extractor_reset_statistics(feature_extractor *extractor);
````

**Parameter**

* `extractor`: feature extractor handle.

**Return Values**

* None

## 2. Feature Inference APIs

### inference_init

**Function**

Creates and initializes an inference engine.

**Function API**

````bash
inference_engine *inference_init(const inference_config *config);
````

**Parameter**

* `config_file`: inference configuration (including the configuration file path).

**Return Values**

* A successful API call returns the inference engine handle. A failed API call returns `NULL`.

### inference_destroy

**Function**

Destroys an inference engine.

**Function API**

````bash
void inference_destroy(inference_engine *engine);
````

**Parameter**

* `engine`: inference engine handle.

**Return Values**

* None

### inference_predict

**Function**

Performs batch inference.

**Function API**

````bash
int inference_predict(inference_engine *engine, const feature_vector_list *features, inference_result *result);
````

**Parameters**

* `engine`: inference engine handle.
* `features`: feature vector list.
* `result`: output inference result.

**Return Values**

* Status code: `INFERENCE_SUCCESS` indicates that the operation is successful, and `INFERENCE_ERROR` indicates that the operation fails.
