# 
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# 

from pathlib import Path

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