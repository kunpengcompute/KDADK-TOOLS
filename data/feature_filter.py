# 
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# 

import os
import sys
import yaml
import pandas as pd

# 获取当前根目录下所有的csv文件
def get_current_csvs(current_dir):
    csv_files = []
    for root, dirs, files in os.walk(current_dir):  # root: 当前遍历的目录
        for filename in files:
            if filename.endswith('.csv'):
                filepath = os.path.join(root, filename)  # 完整路径
                csv_files.append(filepath)
    
    return csv_files

def filter_csv_files_by_packets(csv_files, filter_packets): 
    for file in csv_files:
        print(f"正在处理文件: {file}")
        
        try:
            df = pd.read_csv(file)
            
            if 'send_packet_nums' in df.columns and 'receive_packet_nums' in df.columns:
                df['total_packets_cnt'] = df['send_packet_nums'] + df['receive_packet_nums']
            else:
                print(f"文件 {file} 缺少send_packet_nums列或receive_packet_nums列，跳过处理")
                continue
            
            # 先保留符合条件的行索引
            rows_to_keep = df[df['total_packets_cnt'] >= filter_packets].index
            
            # 删除不符合条件的行
            df = df.loc[rows_to_keep]
    
            df = df.drop(columns=['total_packets_cnt'])
            
            # 6. 保存处理后的文件（覆盖原文件）
            df.to_csv(file, index=False)
            print(f"文件 {file} 处理完成")
            
        except Exception as e:
            print(f"处理文件 {file} 时出错: {str(e)}")

def validate_column_indices(columns_to_remove, max_columns=None):
    for i, col_idx in enumerate(columns_to_remove):
        if not isinstance(col_idx, int):
            raise ValueError(f"列索引必须是整数，但第 {i+1} 个值是 {type(col_idx).__name__}: {col_idx}")
        
        if col_idx < 0:
            raise ValueError(f"列索引必须是非负整数，但第 {i+1} 个值是负数: {col_idx}")
        
        if max_columns is not None and col_idx >= max_columns:
            raise ValueError(f"列索引 {col_idx} 超出范围 [0, {max_columns-1}]")
    
    return True

def remove_columns_from_csv(csv_files, columns_to_remove):
    if not columns_to_remove:
        print("columns_to_remove 为空，跳过列删除操作")
        return

    try:
        validate_column_indices(columns_to_remove)
    except ValueError as e:
        print(f"配置错误: {e}")
        return
    
    for filepath in csv_files:
        try:
            df = pd.read_csv(filepath)

            num_columns = len(df.columns)
            print(f"文件 {filepath} 有 {num_columns} 列")
            
            try:
                validate_column_indices(columns_to_remove, num_columns)
            except ValueError as e:
                print(f"文件 {filepath} 处理失败: {e}")
                continue  # 跳过这个文件，继续处理下一个
            
            # 按降序排列索引，避免删除时索引偏移问题
            sorted_columns = sorted(columns_to_remove, reverse=True)
            
            for col_idx in sorted_columns:
                column_name = df.columns[col_idx]
                df.drop(columns=[column_name], inplace=True)
                print(f"  删除列 {col_idx}: '{column_name}'")
            
            # 保存修改后的文件（覆盖原文件）
            df.to_csv(filepath, index=False)
            print(f"已处理文件: {filepath}")
            
        except Exception as e:
            print(f"处理文件 {filepath} 时出错: {str(e)}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 feature_filter.py <path_to_config> <path_to_data_folder>")
        sys.exit(1)
    config_file = sys.argv[1]
    current_dir = sys.argv[2]
    try:
        with open(config_file, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
        
        # 处理 columns_to_remove 字段（可选，默认为空列表）
        if 'columns_to_remove' in config:
            columns_to_remove = config['columns_to_remove']
            # 字段存在时进行严格验证
            if not isinstance(columns_to_remove, list):
                raise ValueError("columns_to_remove must be a list.")
        else:
            # 字段缺失时使用默认值
            columns_to_remove = []
            print("配置文件中未找到 'columns_to_remove' 字段，使用默认值: []")
        
        # 处理 filter_packets 字段（可选，默认为 0）
        if 'filter_packets' in config:
            filter_packets = config['filter_packets']
            # 字段存在时进行严格验证
            if not isinstance(filter_packets, int) or filter_packets < 0:
                raise ValueError("filter_packets must be a non-negative integer.")
        else:
            # 字段缺失时使用默认值
            filter_packets = 0
            print("配置文件中未找到 'filter_packets' 字段，使用默认值: 0")
        
    except PermissionError:
        raise PermissionError("No permission to read the config.yaml file. Please check the file permissions.")
    except yaml.YAMLError:
        raise yaml.YAMLError("The YAML file format is incorrect.")
    except ValueError as e:
        raise ValueError(f"Configuration value error - {str(e)}") from None
    except Exception as e:
        raise Exception(f"An unexpected error occurred while reading the configuration file. - {str(e)}") from None

    csv_files = get_current_csvs(current_dir)
    
    # 只有当 filter_packets > 0 时才执行过滤
    if filter_packets > 0:
        print(f"过滤packets: {filter_packets}")
        filter_csv_files_by_packets(csv_files, filter_packets)
    else:
        print(f"filter_packets 为 {filter_packets}，跳过 packets 过滤操作")
    
    # 只有当 columns_to_remove 非空时才执行列删除
    if columns_to_remove:
        print(f"过滤columns: {columns_to_remove}")
        remove_columns_from_csv(csv_files, columns_to_remove)
    else:
        print("columns_to_remove 为空，跳过 columns 过滤操作")
    
    print("所有文件处理完成")