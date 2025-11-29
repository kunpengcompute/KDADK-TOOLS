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
import shutil

# 定义源目录
source_dir = 'data'

# 遍历data目录下的每个文件夹
for folder_name in os.listdir(source_dir):
    folder_path = os.path.join(source_dir, folder_name)
    
    # 确保是文件夹
    if os.path.isdir(folder_path):
        # 创建csv和pcap文件夹
        csv_dir = os.path.join(folder_path, 'csv')
        pcap_dir = os.path.join(folder_path, 'pcap')
        
        os.makedirs(csv_dir, exist_ok=True)
        os.makedirs(pcap_dir, exist_ok=True)
        
        # 遍历文件夹中的文件
        for file_name in os.listdir(folder_path):
            file_path = os.path.join(folder_path, file_name)
            
            # 判断文件类型并移动文件
            if file_name.endswith('.csv'):
                shutil.move(file_path, os.path.join(csv_dir, file_name))
            elif file_name.endswith('.pcap'):
                shutil.move(file_path, os.path.join(pcap_dir, file_name))

print("文件分类完成！")