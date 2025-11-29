--| labeling.py                 -- 打标签工具，具体用法可以参考下述描述
--| config_label.yaml           -- 配置文件，可以配置打标签工具具体参数
--| results                     -- 打标签工具结果输出位置，可在配置文件中修改
--| output                      -- 聚类预测结果输出文件夹，可在配置文件中修改


python labeling.py -h

usage: labeling.py [-h] [--mode {train,inference}] [--config CONFIG]

打标签工具

options:
  -h, --help            show this help message and exit
  --mode {train,inference}
                        运行模式: train(训练) 或 inference(推理)
  --config CONFIG       配置文件路径 (默认: config_label.yaml)

使用示例:
  python labeling.py --mode train                    # 使用默认配置文件训练
  python labeling.py --mode inference                # 使用默认配置文件推理，默认使用每个文件夹下的一个csv文件
  python labeling.py --mode train --config ./config_label.yaml  # 指定配置文件