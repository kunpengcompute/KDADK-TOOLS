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

import sys
import logging
from typing import List
from sklearn.metrics import classification_report, accuracy_score
import yaml
import joblib
import pandas as pd
import numpy as np
import os
from datetime import datetime
from utils import resolve_path

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

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


def load_and_prepare_data(data_paths: List[List[str]], scaler_path: str):
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
    # X = combined_data.drop('label', axis=1)
    y = combined_data['label']
    raw_bow = None
    if 'raw_bow' in combined_data.columns:
        raw_bow = combined_data['raw_bow']
        X = combined_data.drop(['label', 'raw_bow'], axis=1)
    else:
        X = combined_data.drop('label', axis=1)

    # standardization
    try:
        scaler = joblib.load(scaler_path)
        X_scaled = scaler.transform(X)
    except FileNotFoundError as e:
        raise FileNotFoundError(f"{scaler_path} file not found") from e
    except PermissionError as e:
        raise PermissionError(f"No permission to load the {scaler_path} file. Please check the file permissions.") from e
    except Exception as e:
        raise RuntimeError(f"Error occurred while standardizing data.") from e

    return X_scaled, y, X, raw_bow


def predict_new_data(params):
    """
    Use the trained model to predict new data.
    
    args:
        model: Trained Random Forest Model
        X_scaled: Data to be predicted
        y: Expected predict result
        X_original: Original features (before scaling)
        raw_bow: Raw bow column if exists
        anomaly_detect_switch: Whether to enable anomaly detection
        anomaly_detect_limit: Threshold for anomaly detection
        report_output_path: Path to save classification report
        prediction_output_path: Path to save prediction results
    """
    model = params['model']
    X_scaled = params['X_scaled']
    y = params['y']
    X_original = params['X_original']
    raw_bow = params['raw_bow']
    anomaly_detect_switch = params['anomaly_detect_switch']
    anomaly_detect_limit = params['anomaly_detect_limit']
    report_output_path = params['report_output_path']
    prediction_output_path = params['prediction_output_path']
    y_pred = model.predict(X_scaled)

    if anomaly_detect_switch:
        # Obtain the predicted probabilities for each category
        probs = model.predict_proba(X_scaled)  
        max_probs = np.max(probs, axis=1)
        is_anomaly = max_probs < anomaly_detect_limit
        # Mark the predicted labels of abnormal samples as -1.
        y_pred = np.where(is_anomaly, -1, y_pred)  
    
    # Generate classification report
    report = classification_report(y, y_pred, digits=4)
    accuracy = accuracy_score(y, y_pred)
    
    logging.info('Model evaluation result:\n%s', report)
    logging.info(f"Accuracy: {accuracy}")
    
    # Save classification report to txt file
    try:
        # Create directory if not exists
        report_dir = os.path.dirname(report_output_path)
        if report_dir and not os.path.exists(report_dir):
            os.makedirs(report_dir)
        
        with open(report_output_path, 'w', encoding='utf-8') as f:
            f.write("=" * 80 + "\n")
            f.write(f"Classification Report\n")
            f.write(f"Generated at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write("=" * 80 + "\n\n")
            f.write(report)
            f.write(f"\n\nAccuracy: {accuracy:.4f}\n")
            f.write("=" * 80 + "\n")
        
        logging.info(f"Classification report saved to: {report_output_path}")
    except Exception as e:
        logging.error(f"Failed to save classification report - {str(e)}")
    
    # Save prediction results to csv file
    try:
        # Create directory if not exists
        pred_dir = os.path.dirname(prediction_output_path)
        if pred_dir and not os.path.exists(pred_dir):
            os.makedirs(pred_dir)
        
        # Create DataFrame with original features
        result_df = pd.DataFrame(X_original)
        
        # Add raw_bow column if exists
        if raw_bow is not None:
            result_df['raw_bow'] = raw_bow.values
        
        # Add prediction and true label columns
        result_df['predicted_label'] = y_pred
        result_df['true_label'] = y.values
        
        # Save to CSV
        result_df.to_csv(prediction_output_path, index=False, encoding='utf-8')
        
        logging.info(f"Prediction results saved to: {prediction_output_path}")
        logging.info(f"Total samples: {len(result_df)}")
        
        # Log prediction summary
        correct_predictions = (y_pred == y.values).sum()
        total_predictions = len(y_pred)
        logging.info(f"Correct predictions: {correct_predictions}/{total_predictions}")
        
    except Exception as e:
        logging.error(f"Failed to save prediction results - {str(e)}")


def KDADK_Evaluation(config_file):
    logging.info("Loaded config file : %s", config_file)
    # File Path and Labels- read from config.yaml
    try:
        with open(config_file, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
        data_paths = config['evaluation_data_paths']
        anomaly_detect_switch = 0
        anomaly_detect_limit = 0
        
        # Get output paths from config, with default values
        report_output_path = resolve_path(config['classification_report_file'])
        prediction_output_path = resolve_path(config['predictions_detail_file'])
        model_path = resolve_path(config['model_path_pkl'])
        scaler_path = resolve_path(config['scaler_path_pkl'])

    except PermissionError:
        logging.error(f"No permission to read the {config_file} file. Please check the file permissions.")
        return -1
    except yaml.YAMLError:
        logging.error("The YAML file format is incorrect.")
        return -1
    except KeyError as e:
        logging.error(f"Missing necessary configuration items - {str(e)}")
        return -1
    except Exception as e:
        logging.error(f"An unexpected error occurred while reading the configuration file. - {str(e)}")
        return -1

    if (not isinstance(anomaly_detect_switch, int)) or (not isinstance(anomaly_detect_limit, (int, float))):
        logging.error("anomaly detect config must be digital")
        return -1

    # Load and prepare test data
    X_scaled, y, X_original, raw_bow = load_and_prepare_data(data_paths, scaler_path)
    
    # Load Model
    try:
        rf_model = joblib.load(model_path)
    except FileNotFoundError:
        logging.error(f"{model_path} file not found")
        return -1
    except PermissionError:
        logging.error(f"No permission to load the {model_path} file. Please check the file permissions.")
        return -1
    except Exception as e:
        logging.error(f"An unexpected error occurred while loading the random forest model. - {str(e)}")
        return -1

    params = {
        'model': rf_model,
        'X_scaled': X_scaled,
        'y': y,
        'X_original': X_original,
        'raw_bow': raw_bow,
        'anomaly_detect_switch': anomaly_detect_switch,
        'anomaly_detect_limit': anomaly_detect_limit,
        'report_output_path': report_output_path,
        'prediction_output_path': prediction_output_path
    }

    predict_new_data(params)

    return 0

if __name__ == "__main__":
    if len(sys.argv) != 2:
        logging.info("Usage: python3 evaluation.py <path_to_config>")
        sys.exit(1)
    config_file = sys.argv[1]
    res = KDADK_Evaluation(config_file)
    if res == -1:
        print("Error: Inference failed.")