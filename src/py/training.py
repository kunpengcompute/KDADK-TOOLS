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

import logging
from typing import List
from sklearn.ensemble import RandomForestClassifier
from sklearn.preprocessing import StandardScaler
from skl2onnx import convert_sklearn
from skl2onnx.common.data_types import FloatTensorType
import yaml
import joblib
import json
import pandas as pd
import sys
import os
from utils import resolve_path

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')


def create_directory(path):
    """创建目录（如果不存在）"""
    if not os.path.exists(path):
        os.makedirs(path)
        logging.info(f"创建目录: {path}")
    else:
        logging.info(f"结果保存路径: {path}")

def _load_and_concat(file_paths: List[str]):
    """Load and concatenate multiple CSV files"""
    dfs = []
    for file_path in file_paths:
        try:
            df = pd.read_csv(resolve_path(file_path))
            dfs.append(df)
        except FileNotFoundError:
            logging.warning(f"{file_path} file not found. Skipping this file.")
            continue
        except PermissionError:
            logging.warning(f"No permission to load {file_path}. Skipping this file.")
            continue
        except pd.errors.EmptyDataError:
            logging.warning(f"File '{file_path}' is empty. Skipping this file.")
            continue
        except Exception as e:
            logging.warning(f"Error occurred while reading {file_path}: {str(e)}. Skipping this file.")
            continue
    if not dfs:
        raise RuntimeError("Error: No data was successfully loaded from any of the provided CSV files.")

    return pd.concat(dfs, axis=0, ignore_index=True)


def load_and_prepare_data(data_paths: List[List[str]]):
    """
     Load and prepare the data, and add labels to the dataset.
    """
    labels = [i for i in range(len(data_paths))]

    all_data = []
    for paths, label in zip(data_paths, labels):
        class_data = _load_and_concat(paths)
        class_data['label'] = label
        all_data.append(class_data)


    combined_data = pd.concat(all_data, axis=0)
    combined_data = combined_data.sample(frac=1).reset_index(drop=True)

    # Separate features and labels
    X = combined_data.drop('label', axis=1)
    y = combined_data['label']
    if 'raw_bow' in combined_data.columns:
        raw_bow = combined_data['raw_bow']
        X = combined_data.drop(['label', 'raw_bow'], axis=1)
    else:
        X = combined_data.drop('label', axis=1)

    return X, y


def train_random_forest(X, y, random_state=42):
    """
    Train a Random Forest Classifier
    
    args:
        X: Feature data
        y: labels
        random_state: Random Seed
        
    return:
        Trained model
    """
    # Data standardization (returns DataFrame instead of NumPy array)
    scaler = StandardScaler().set_output(transform="pandas")
    X_scaled = scaler.fit_transform(X)

    # Initialize the Random Forest Classifier
    rf = RandomForestClassifier(
        n_estimators=100,
        max_depth=10,
        min_samples_split=5,
        min_samples_leaf=2,
        random_state=random_state,
        oob_score=True
    )
    
    # Train the model
    rf.fit(X_scaled, y)
    logging.info("OOB Score: %s", rf.oob_score_)

    # print feature importance
    feature_importance = rf.feature_importances_
    features_df = pd.DataFrame({
        'Feature': X_scaled.columns,
        'Importance': feature_importance
    }).sort_values('Importance', ascending=False)
    logging.info("Top 30 features:\n%s", features_df.head(30))
    return rf, scaler


def KDADK_Train(config_file):
    logging.info("Loaded config file : %s", config_file)
    # File Path and Labels- read from config.yaml
    try:
        with open(config_file, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
        data_paths = config['training_data_paths']
    except PermissionError:
        logging.error("No permission to read the config.yaml file. Please check the file permissions.")
        return
    except yaml.YAMLError:
        logging.error("The YAML file format is incorrect.")
        return
    except KeyError as e:
        logging.error(f"Missing necessary configuration items - {str(e)}")
        return
    except Exception as e:
        logging.error(f"An unexpected error occurred while reading the configuration file. - {str(e)}")
        return

    X, y = load_and_prepare_data(data_paths)
    logging.info("Start training the model...")
    rf_model, scaler = train_random_forest(X, y)

    # Save the model
    output_dir = resolve_path(config['output_dir'])
    create_directory(output_dir)

    # 拼接相对路径
    model_path_pkl = resolve_path(config['model_path_pkl'])
    try:
        joblib.dump(rf_model, model_path_pkl)
    except PermissionError:
        logging.error("No permission to dump the model_classifier.pkl. Please check the file permissions.")
        return
    except IOError as e:
        logging.error(f"{str(e)}")
        return
    except Exception as e:
        logging.error(f"An unexpected error occurred while saving the model. {str(e)}")
        return

    logging.info(f"Model saved as {model_path_pkl}")

    # Save in ONNX format
    initial_type = [('float_input', FloatTensorType([None, X.shape[1]]))]
    onnx_model = convert_sklearn(rf_model, initial_types=initial_type, options={id(rf_model): {'zipmap': False}})
    model_path_onnx = resolve_path(config['model_path_onnx'])

    with open(model_path_onnx, "wb") as f:
        f.write(onnx_model.SerializeToString())
    logging.info(f"Model saved as {model_path_onnx}")

    # Save the scaler
    # Save the standardizer for subsequent predictions.
    scaler_path_pkl = resolve_path(config['scaler_path_pkl'])

    try:
        joblib.dump(scaler, scaler_path_pkl)
    except PermissionError as e:
        raise PermissionError("No permission to dump the scaler.pkl. Please check the file permissions.") from e
    except IOError as e:
        raise IOError(f"IO Error while saving the model.: {str(e)}") from e
    except Exception as e:
        raise RuntimeError(f"Error: An unexpected error occurred while saving the model. {str(e)}") from e
    logging.info(f"Scaler saved as {scaler_path_pkl}")

    # 提取参数
    scaler_params = {
        'mean': scaler.mean_.tolist(),
        'scale': scaler.scale_.tolist(),
        'feature_names': scaler.feature_names_in_.tolist() if hasattr(scaler, 'feature_names_in_') else []
    }

    # 保存为 JSON 文件（C++ 易于读取的格式）
    scaler_path_json = resolve_path(config['scaler_path_json'])

    with open(scaler_path_json, 'w') as f:
        json.dump(scaler_params, f, indent=4)

    logging.info(f"StandardScaler 参数已导出到 {scaler_path_json}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        logging.info("Usage: python3 training.py <path_to_config>")
        sys.exit(1)
    config_file = sys.argv[1]
    KDADK_Train(config_file)