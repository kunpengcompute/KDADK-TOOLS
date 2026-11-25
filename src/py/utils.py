# 
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# 

from pathlib import Path
import hashlib
import hmac
import io


def load_verified_joblib(path, expected_sha256):
    """Load only artifacts pinned in trusted configuration, never arbitrary pickle.

    The digest must come from a trusted producer, independently of the artifact.
    Hashing does not make an untrusted model safe. Deserialize the exact bytes
    checked here so replacing the path after verification cannot bypass the check.
    """
    if (not isinstance(expected_sha256, str) or len(expected_sha256) != 64
            or any(c not in '0123456789abcdefABCDEF' for c in expected_sha256)):
        raise ValueError('A trusted 64-character SHA-256 digest is required for each model/scaler.')
    payload = Path(path).read_bytes()
    if not hmac.compare_digest(hashlib.sha256(payload).hexdigest(), expected_sha256.lower()):
        raise ValueError(f'Artifact SHA-256 mismatch: {path}; refusing to deserialize.')
    import joblib
    return joblib.load(io.BytesIO(payload))

def resolve_path(path_str, base_dir=None):
    """
    解析路径，支持相对路径和绝对路径
    
    Args:
        path_str: 路径字符串
        base_dir: 相对路径的基准目录，默认为脚本所在目录
    
    Returns:
        Path: 解析后的绝对路径
    """
    path = Path(path_str)
    
    # 如果已经是绝对路径，直接返回
    if path.is_absolute():
        return path.resolve()
    
    # 如果是相对路径，基于 base_dir 解析
    if not base_dir:
        base_dir = Path(__file__).parent.parent.parent.absolute()
    else:
        base_dir = Path(base_dir).absolute()
    return (base_dir / path).resolve()
