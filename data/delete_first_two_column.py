# 
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
import pandas as pd

def remove_columns_from_csv():
    # 获取当前目录
    current_dir = os.getcwd()
    
    csv_files = []
    for root, dirs, files in os.walk(current_dir):  # root: 当前遍历的目录
        for filename in files:
            if filename.endswith('.csv'):
                filepath = os.path.join(root, filename)  # 完整路径
                csv_files.append(filepath)


    # 遍历当前目录下的所有文件
    for filepath in csv_files:
        try:
            # 读取CSV文件
            df = pd.read_csv(filepath)
            
            # 检查是否有足够多的列
            if len(df.columns) < 2:
                print(f"文件 {filepath} 不足2列，跳过处理")
                continue
            
            # 删除A列和B列（索引为0和1的列）
            df.drop(df.columns[[0, 1]], axis=1, inplace=True)
            
            # 保存修改后的文件（覆盖原文件）
            df.to_csv(filepath, index=False)
            print(f"已处理文件: {filepath}")
            
        except Exception as e:
            print(f"处理文件 {filepath} 时出错: {str(e)}")

if __name__ == "__main__":
    remove_columns_from_csv()