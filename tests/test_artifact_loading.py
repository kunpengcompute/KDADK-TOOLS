"""Run with: python -m unittest discover -s tests -p 'test_*.py'."""
import hashlib
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

import joblib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'src' / 'py'))
from utils import load_verified_joblib


class ArtifactLoadingTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / 'model.pkl'

    def dump(self, obj, compress=0):
        joblib.dump(obj, self.path, compress=compress)
        return hashlib.sha256(self.path.read_bytes()).hexdigest()

    def test_valid_artifacts_including_compression(self):
        for compression in (0, 3):
            with self.subTest(compression=compression):
                digest = self.dump({'model': [1, 2, 3]}, compression)
                self.assertEqual(load_verified_joblib(self.path, digest.upper()),
                                 {'model': [1, 2, 3]})

    def test_missing_or_malformed_pin_never_deserializes(self):
        self.dump({'model': 1})
        with patch('joblib.load') as loader:
            for digest in (None, '', 123, '0' * 63, 'g' * 64, '0' * 64 + '\n'):
                with self.subTest(digest=digest), self.assertRaises(ValueError):
                    load_verified_joblib(self.path, digest)
            loader.assert_not_called()

    def test_replaced_artifact_is_rejected(self):
        digest = self.dump({'model': 1})
        self.dump({'model': 2})
        with patch('joblib.load') as loader, self.assertRaises(ValueError):
            load_verified_joblib(self.path, digest)
        loader.assert_not_called()

    def test_malicious_reduce_is_blocked_before_execution(self):
        marker = Path(self.directory.name) / 'executed'

        class Payload:
            def __reduce__(self):
                # Harmless local marker proves deserialization executed code.
                return (mkdir_marker, (str(marker),))

        trusted_digest = self.dump({'model': 1})
        self.dump(Payload())
        with self.assertRaises(ValueError):
            load_verified_joblib(self.path, trusted_digest)
        self.assertFalse(marker.exists())
        # Control: the same artifact executes its reducer via unguarded joblib.
        joblib.load(self.path)
        self.assertTrue(marker.is_dir())

    def test_deserializer_receives_verified_snapshot(self):
        digest = self.dump({'model': 1})
        original_load = joblib.load

        def replace_before_load(snapshot):
            self.dump({'model': 2})
            return original_load(snapshot)

        with patch('joblib.load', side_effect=replace_before_load):
            self.assertEqual(load_verified_joblib(self.path, digest), {'model': 1})


def mkdir_marker(path):
    Path(path).mkdir()


@unittest.skipUnless(importlib.util.find_spec('sklearn') and importlib.util.find_spec('pandas'),
                     'Requires the ML runtime dependencies')
class InferenceCompatibilityTests(unittest.TestCase):
    def test_evaluation_and_labeling_with_real_estimators(self):
        import numpy as np
        import pandas as pd
        import yaml
        from sklearn.cluster import KMeans
        from sklearn.ensemble import RandomForestClassifier
        from sklearn.preprocessing import StandardScaler, MinMaxScaler
        from evaluation import KDADK_Evaluation

        label_path = Path(__file__).resolve().parents[1] / 'tools' / 'labeling' / 'labeling.py'
        spec = importlib.util.spec_from_file_location('kdadk_labeling_test', label_path)
        labeling = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(labeling)
        X = pd.DataFrame({'a': [0., 1., 10., 11.], 'b': [1., 2., 11., 12.]})
        y = [0, 0, 1, 1]
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            def save(name, obj):
                path = root / name
                joblib.dump(obj, path)
                return str(path), hashlib.sha256(path.read_bytes()).hexdigest()

            scaler = StandardScaler().fit(X)
            model = RandomForestClassifier(n_estimators=5, random_state=42).fit(scaler.transform(X), y)
            model_path, model_digest = save('rf.pkl', model)
            scaler_path, scaler_digest = save('standard.pkl', scaler)
            csv_paths = []
            for i in range(2):
                path = root / f'class{i}.csv'
                X.iloc[i * 2:i * 2 + 2].to_csv(path, index=False)
                csv_paths.append([str(path)])
            config = dict(evaluation_data_paths=csv_paths, model_path_pkl=model_path,
                          scaler_path_pkl=scaler_path, model_sha256=model_digest,
                          scaler_sha256=scaler_digest,
                          classification_report_file=str(root / 'report.txt'),
                          predictions_detail_file=str(root / 'predictions.csv'))
            config_path = root / 'config.yaml'
            config_path.write_text(yaml.safe_dump(config), encoding='utf-8')
            self.assertEqual(KDADK_Evaluation(str(config_path)), 0)
            predictions = pd.read_csv(root / 'predictions.csv')
            np.testing.assert_array_equal(predictions.predicted_label, predictions.true_label)

            scaler = MinMaxScaler().fit(X)
            model = KMeans(n_clusters=2, n_init=1, random_state=42).fit(scaler.transform(X))
            model_path, model_digest = save('kmeans.pkl', model)
            scaler_path, scaler_digest = save('minmax.pkl', scaler)
            result = labeling.predict_with_kmeans_model(
                model_path, scaler_path, X, model_digest, scaler_digest)
            np.testing.assert_array_equal(result['cluster_predictions'], model.predict(scaler.transform(X)))
            with self.assertRaises(ValueError):
                labeling.predict_with_kmeans_model(model_path, scaler_path, X)


if __name__ == '__main__':
    unittest.main()
