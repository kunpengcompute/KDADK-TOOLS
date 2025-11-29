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

def process_csv_files():
    # 递归获取当前目录及子目录下所有.csv文件
    csv_files = []
    for root, dirs, files in os.walk('.'):  # '.'表示当前目录
        for file in files:
            if file.endswith('.csv'):
                csv_files.append(os.path.join(root, file))  # 保留完整路径
    
    for file in csv_files:
        print(f"正在处理文件: {file}")
        
        try:
            # 1. 读取CSV文件
            df = pd.read_csv(file)
            
            # 2. 在最右侧新增一列X，计算公式为=E:E+BO:BO
            # 注意：列名可能是'E'或'E列'，需要根据实际情况调整
            # 假设列名就是单个字母
            if 'send_packet_nums' in df.columns and 'receive_packet_nums' in df.columns:
                df['total_packets_cnt'] = df['send_packet_nums'] + df['receive_packet_nums']
            else:
                print(f"文件 {file} 缺少send_packet_nums列或receive_packet_nums列，跳过处理")
                continue
            
            # 3. 筛选X列小于16的行
            # 先保留符合条件的行索引
            rows_to_keep = df[df['total_packets_cnt'] >= 16].index
            
            # 4. 删除不符合条件的行
            df = df.loc[rows_to_keep]
            
            # 5. 删除X列
            df = df.drop(columns=['total_packets_cnt'])
            
            # 6. 保存处理后的文件（覆盖原文件）
            df.to_csv(file, index=False)
            print(f"文件 {file} 处理完成")
            
        except Exception as e:
            print(f"处理文件 {file} 时出错: {str(e)}")

if __name__ == "__main__":
    process_csv_files()
    print("所有文件处理完成")