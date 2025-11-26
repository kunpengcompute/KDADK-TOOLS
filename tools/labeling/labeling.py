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
import time
import glob
import argparse
import yaml
import sys
import joblib
import numpy as np
import pandas as pd
from sklearn.cluster import KMeans
from sklearn.metrics import silhouette_score
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import MinMaxScaler
from typing import Dict

class Config:
    """配置类，用于加载和管理配置文件"""
    
    def __init__(self, config_path='config.yaml'):
        self.config_path = config_path
        self.config = self.load_config()
    
    def load_config(self):
        """从YAML文件加载配置"""
        try:
            with open(self.config_path, 'r', encoding='utf-8') as f:
                config = yaml.safe_load(f)
            print(f"✓ 成功加载配置文件: {self.config_path}")
            return config
        except FileNotFoundError:
            print(f"✗ 错误: 配置文件 '{self.config_path}' 不存在")
            sys.exit(1)
        except yaml.YAMLError as e:
            print(f"✗ 错误: 配置文件解析失败 - {e}")
            sys.exit(1)
    
    def get(self, key, default=None):
        """获取配置项"""
        keys = key.split('.')
        value = self.config
        for k in keys:
            if isinstance(value, dict):
                value = value.get(k, default)
            else:
                return default
        return value
    
    def __getitem__(self, key):
        """支持字典式访问"""
        return self.get(key)

def create_directory(path):
    """创建目录（如果不存在）"""
    if not os.path.exists(path):
        os.makedirs(path)
        print(f"创建目录: {path}")
    else:
        print(f"结果保存路径: {path}")

def load_data(target_files, inference=False):
    dfs = []
    for file_path in target_files:
        try:
            df = pd.read_csv(file_path)
            dfs.append(df)
            print(f"  已加载: {os.path.basename(file_path)}")
            if inference:
                break  # 推理模式下只加载一个文件
        except Exception as e:
            print(f"  加载文件 {os.path.basename(file_path)} 时出错: {str(e)}")

    if not dfs:
        raise ValueError("没有任何数据框被成功加载")
    
    return dfs

def load_and_concat(folder_path: str, inference=False) -> pd.DataFrame:
    """
    加载并拼接指定文件夹中特定命名模式的CSV文件
    """
    all_csv_files = glob.glob(os.path.join(folder_path, "*.csv"))
    
    if not all_csv_files:
        print(f"警告: 在 {folder_path} 中未找到CSV文件")
        return pd.DataFrame()  # 返回空DataFrame
    
    dfs = load_data(all_csv_files, inference)
    return pd.concat(dfs, axis=0, ignore_index=True)

def load_and_prepare_data(data_paths: list, labels: list = None, inference=False) -> tuple:
    """
    加载并准备数据，为多个数据源添加标签
    
    Args:
        data_paths: 数据路径列表，每个元素是一个字典，包含 'path' 和可选的 'name'
                   例如: [
                       {'path': './data/tencent', 'name': 'video'},
                       {'path': './data/wenxiaoyan', 'name': 'text'},
                       {'path': './data/wymusic', 'name': 'music'}
                   ]
                   或简单的路径列表: ['./data/tencent', './data/wenxiaoyan']
        labels: 可选的标签列表，如果为None则自动分配0, 1, 2...
        balanced: 是否平衡数据（预留参数，可后续实现）
    
    Returns:
        X: 特征数据
        y: 标签数据
        raw_bow: raw_bow列数据
    """
    print("=" * 60)
    print("开始加载数据集...")
    print("=" * 60)
    
    # 标准化输入格式
    normalized_paths = []
    for i, item in enumerate(data_paths):
        if isinstance(item, dict):
            normalized_paths.append({
                'path': item.get('path'),
                'name': item.get('name', f'dataset_{i}')
            })
        elif isinstance(item, str):
            normalized_paths.append({
                'path': item,
                'name': f'dataset_{i}'
            })
        else:
            raise ValueError(f"数据路径格式错误: {item}")
    
    # 加载数据并添加标签
    valid_datasets = []
    label_mapping = {}
    
    for idx, path_info in enumerate(normalized_paths):
        path = path_info['path']
        name = path_info['name']
        
        print(f"\n[{idx + 1}/{len(normalized_paths)}] 加载 {name} 数据...")
        print(f"  路径: {path}")
        
        try:
            data = load_and_concat(path, inference)
            
            if not data.empty:
                data = data.copy()
                
                # 分配标签
                if labels is not None and idx < len(labels):
                    label = labels[idx]
                else:
                    label = len(valid_datasets)  # 动态分配标签
                
                data['label'] = label
                valid_datasets.append(data)
                label_mapping[name] = label
                
                # print(f"  ✓ {name} 数据: {len(data)} 条记录 (标签: {label})")
                print(f"  ✓ {name} 数据: {len(data)} 条记录")
            else:
                print(f"  ✗ 警告: {name} 数据集为空，跳过")
                
        except Exception as e:
            print(f"  ✗ 加载 {name} 数据时出错: {str(e)}")
            continue
    
    # 检查是否有有效数据
    if not valid_datasets:
        raise ValueError("所有数据集都为空或加载失败！")
    
    print("\n" + "=" * 60)
    print("数据加载完成，开始合并和处理...")
    print("=" * 60)
    
    # 合并数据集
    combined_data = pd.concat(valid_datasets, axis=0, ignore_index=True)
    
    # 打乱数据顺序
    combined_data = combined_data.sample(frac=1, random_state=42).reset_index(drop=True)
    
    # 检查必要的列是否存在
    if 'label' not in combined_data.columns:
        raise ValueError("合并后的数据缺少 'label' 列")
    
    # 分离特征和标签
    X = combined_data.drop('label', axis=1)
    
    # 处理 raw_bow 列
    raw_bow = None
    if 'raw_bow' in X.columns:
        raw_bow = X['raw_bow'].copy()
        X = X.drop('raw_bow', axis=1)
        print("  ✓ 已提取 raw_bow 列")
    else:
        print("  ⚠ 未找到 raw_bow 列")
    
    y = combined_data['label'].values  # 转换为numpy数组
    
    # 打印统计信息
    print("\n" + "=" * 60)
    print("数据集统计信息")
    print("=" * 60)
    print(f"总数据量: {len(combined_data)}")
    print(f"特征数量: {X.shape[1]}")
    
    print("=" * 60 + "\n")
    
    return X, y, label_mapping, raw_bow

def print_evaluation_report(best_k, train_silhouette, test_silhouette, inertia, 
                          n_train, n_test, results_dir, model_name):
    """
    打印评估报告
    """
    print("\n" + "="*50)
    print("K-Means 模型评估报告")
    print("="*50)
    print(f"· 使用的聚类数 k: {best_k}")
    print(f"· 训练集轮廓系数: {train_silhouette:.4f}")
    print(f"· 测试集轮廓系数: {test_silhouette:.4f}")
    print(f"· 训练集惯性: {inertia:.4f}")
    print(f"· 训练集样本数: {n_train}")
    print(f"· 测试集样本数: {n_test}")
    print(f"· 结果保存路径: {results_dir}/")
    print("="*50)

def train_model(X, n_clusters=6, test_size=0.3, random_state=42, model_name='kmeans', results_dir="results"):
    """
    使用 KMeans 进行聚类训练和预测
    """
    
    create_directory(results_dir)
    print("数据预处理...")
    numeric_columns = X.select_dtypes(include=[np.number]).columns
    if len(numeric_columns) == 0:
        raise ValueError("数据中没有数值特征！")
    
    X_numeric = X[numeric_columns]
    X_numeric = X_numeric.fillna(X_numeric.mean())
    
    # 数据标准化
    scaler = MinMaxScaler()
    X_scaled = scaler.fit_transform(X_numeric)
    
    # 2. 划分训练集和测试集
    X_train, X_test = train_test_split(
        X_scaled, 
        test_size=test_size, 
        random_state=random_state
    )
    print(f"训练集数据：{X_train.shape[0]}, 测试集：{X_test.shape[0]}")

    # 3. 验证指定的聚类数
    max_clusters = min(len(X_train)//2, len(X_train))
    if n_clusters > max_clusters:
        print(f"警告：指定的聚类数 {n_clusters} 过大，调整为 {max_clusters}")
        n_clusters = max_clusters
    elif n_clusters < 2:
        print(f"警告：指定的聚类数 {n_clusters} 过小，调整为 2")
        n_clusters = 2
    
    print(f"使用指定的聚类数 k = {n_clusters}")

    print(f"训练模型 (k={n_clusters})...")
    # 统计训练时间
    start_time = time.time()
    kmeans = KMeans(n_clusters=n_clusters, random_state=random_state, n_init=10)
    labels_train = kmeans.fit_predict(X_train)
    labels_test = kmeans.predict(X_test)
    train_silhouette = silhouette_score(X_train, labels_train)
    test_silhouette = silhouette_score(X_test, labels_test)
    end_time = time.time()
    print(f"模型训练完成，耗时 {end_time - start_time:.2f} 秒")

    joblib.dump(kmeans, f"{results_dir}/{model_name}_model.pkl")
    joblib.dump(scaler, f"{results_dir}/{model_name}_scaler.pkl")
    print(f"模型已保存到 {results_dir}/{model_name}_model.pkl")
    print(f"标准化器已保存到 {results_dir}/{model_name}_scaler.pkl")

    print_evaluation_report(n_clusters, train_silhouette, test_silhouette, 
                           kmeans.inertia_, len(X_train), len(X_test), 
                           results_dir, model_name)

    return kmeans, X_train, X_test, labels_train, labels_test

def predict_with_kmeans_model(model_path: str, 
                             scaler_path: str, 
                             input_data: pd.DataFrame):
    """
    使用训练好的模型进行聚类预测
    """
    try:
        print("加载模型和预处理器...")
        kmeans_model = joblib.load(model_path)
        scaler = joblib.load(scaler_path)
        
        print("数据预处理...")
        numeric_columns = input_data.select_dtypes(include=[np.number]).columns
        if len(numeric_columns) == 0:
            raise ValueError("数据中没有数值特征！")
        
        X_numeric = input_data[numeric_columns]
        X_numeric = X_numeric.fillna(X_numeric.mean())
        X_scaled = scaler.transform(X_numeric)
        
        print("进行聚类预测...")
        cluster_predictions = kmeans_model.predict(X_scaled)
        
        result = {
            "cluster_predictions": cluster_predictions,
            "n_clusters": kmeans_model.n_clusters,
            "cluster_centers": kmeans_model.cluster_centers_
        }

        return result
        
    except Exception as e:
        print(f"预测过程中出错: {str(e)}")
        raise

def save_clustering_results(input_data: pd.DataFrame, 
                           predictions: Dict, 
                           output_path: str, 
                           label_mapping: Dict,
                           raw_bow: pd.Series = None) -> None:
    """
    将输入数据、真实标签和聚类预测结果保存到CSV文件
    
    Args:
        input_data: 输入的特征数据
        predictions: 预测结果字典
        output_path: 输出文件路径
        label_mapping: 标签映射字典
        raw_bow: raw_bow列数据
    """
    # 创建结果DataFrame
    result_df = input_data.copy()
    
    # 添加聚类预测结果
    result_df['cluster_prediction'] = predictions['cluster_predictions']
    
    # 如果有映射后的预测结果，也添加进去
    if 'mapped_predictions' in predictions:
        result_df['mapped_prediction'] = predictions['mapped_predictions']
        result_df['true_label'] = predictions['true_labels']
        
        # 添加标签名称
        result_df['true_label_name'] = result_df['true_label'].map(label_mapping)
        result_df['mapped_prediction_name'] = result_df['mapped_prediction'].map(label_mapping)
    
    # 添加raw_bow列到最后一列
    if raw_bow is not None:
        # 处理raw_bow中的空值，将NaN转换为空字符串
        raw_bow_processed = raw_bow.fillna('')
        result_df['raw_bow'] = raw_bow_processed
        print(f"已添加raw_bow列，包含 {len(raw_bow_processed)} 个条目")
        
        # 统计raw_bow列的数据情况
        non_empty_count = sum(1 for x in raw_bow_processed if str(x).strip() != '')
        empty_count = len(raw_bow_processed) - non_empty_count
        print(f"raw_bow列统计: 非空数据 {non_empty_count} 条，空数据 {empty_count} 条")
    else:
        print("警告: raw_bow数据为空，将创建空的raw_bow列")
        result_df['raw_bow'] = ''
    
    # 保存结果
        # 如果output_path不包含扩展名，自动添加.csv
    if not output_path.endswith('.csv'):
        output_path = output_path + '.csv'
    
    # 获取目录路径
    output_dir = os.path.dirname(output_path)
    
    # 只有当目录路径不为空时才创建目录
    if output_dir:  # 修改：添加判断
        create_directory(output_dir)
    
    # 保存结果
    result_df.to_csv(output_path, index=False)
    # create_directory(os.path.dirname(output_path))
    # result_df.to_csv(output_path, index=False)
    print(f"聚类结果已保存至: {output_path}")
    print(f"输出文件包含 {len(result_df)} 行数据，{len(result_df.columns)} 列特征")

def training(config):
    """
    训练打标签模型
    """
    data_paths = config.get('train.data_paths')
    print(f'data_paths: {data_paths}')
    model_name = config.get('train.model_name')
    print(f'model_name: {model_name}')
    n_clusters = config.get('train.n_clusters')
    print(f'n_clusters: {n_clusters}')
    test_size = config.get('train.test_size')
    print(f'test_size: {test_size}')
    results_dir = config.get('train.results_dir')
    print(f'results_dir: {results_dir}')
    try:
        # 1. 加载并准备数据
        X, _, _, _ = load_and_prepare_data(data_paths)

        print("开始训练...")
        # 2. 训练模型
        train_model(X, n_clusters=n_clusters, test_size=test_size, model_name=model_name, results_dir=results_dir)
        
    except Exception as e:
        print(f"程序执行出错: {str(e)}")
        import traceback
        traceback.print_exc()

def inferenceing(config):
    """
    推理方法
    """
    data_paths  = config.get('inference.data_paths')
    print(f'data_paths: {data_paths}')
    model_path  = config.get('inference.model_path')
    print(f'model_path: {model_path}')
    scaler_path = config.get('inference.scaler_path')
    print(f'scaler_path: {scaler_path}')
    output_file = config.get('inference.output_file')
    print(f'output_file: {output_file}')
    true_label = config.get('inference.true_label')
    print(f'true_label: {true_label}')

    try:
        X_test, _, label_mapping, raw_bow = load_and_prepare_data(data_paths, inference=True)

        if not os.path.exists(model_path):
            print(f"错误: 模型文件不存在 - {model_path}")
            return
            
        if not os.path.exists(scaler_path):
            print(f"错误: 标准化器文件不存在 - {scaler_path}")
            return

        predictions = predict_with_kmeans_model(
            model_path, scaler_path, X_test
        )
        save_clustering_results(X_test, predictions, output_file, label_mapping, raw_bow)
        
        # 7. 输出预测结果统计
        cluster_preds = predictions['cluster_predictions']
        unique_clusters, counts = np.unique(cluster_preds, return_counts=True)
        print(f"\n聚类结果统计:")
        for cluster, count in zip(unique_clusters, counts):
            percentage = count/len(cluster_preds)*100
            print(f"聚类 {cluster}: {count} 个样本 ({percentage:.2f}%)")

    except Exception as e:
        print(f"程序执行过程中出错: {str(e)}")
        import traceback
        traceback.print_exc()

def main():
    parser = argparse.ArgumentParser(
        description='打标签工具',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
    使用示例:
        python labeling.py --mode train                    # 使用默认配置文件训练
        python labeling.py --mode inference                # 使用默认配置文件推理
        python labeling.py --mode train --config ./config_label.yaml  # 指定配置文件
        """
    )
    parser.add_argument(
        '--mode',
        type=str,
        default='train',
        choices=['train', 'inference'],
        help='运行模式: train(训练) 或 inference(推理)'
    )
    
    parser.add_argument(
        '--config',
        type=str,
        default='config_label.yaml',
        help='配置文件路径 (默认: config_label.yaml)'
    )
    
    args = parser.parse_args()
    config = Config(args.config)
    
    if args.mode == 'train':
        training(config)
    elif args.mode == 'inference':
        inferenceing(config)

if __name__ == '__main__':
    main()