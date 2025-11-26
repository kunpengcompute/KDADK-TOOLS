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

#!/bin/bash

# 默认设置
DEFAULT_CONTAINER_NAME="android_16"
DEFAULT_INTERFERENCE="eth0"
DEFAULT_DURATION=30
DEFAULT_FILENAME="bilibili"

# 初始化变量
CONTAINER_NAME="$DEFAULT_CONTAINER_NAME"
INTERFERENCE="$DEFAULT_INTERFERENCE"
DURATION="$DEFAULT_DURATION"
DOCKER_FILENAME="$DEFAULT_FILENAME"

# 显示帮助信息
show_help() {
    cat << EOF
用法: $0 [选项]

选项:
    -t DURATION     设置抓包持续时间（单位：秒，默认：30秒）
    -n NAME         设置输出文件名前缀（默认：bilibili）
    -c CONTAINER    设置容器名称（默认：android_16）
    -i INTERFACE    设置网络接口（默认：eth0）
    -h              显示此帮助信息

示例:
    $0 -t 30 -n bilibili -c android_16        # 完整参数
    $0 -t 60 -n deepseek                      # 抓包60秒，文件名前缀为deepseek
    $0 -t 45 -c android_17                    # 抓包45秒，指定容器
    $0 -n test                                # 使用默认时间和容器，文件名前缀为test

说明:
    - 抓包文件将保存在 /tmp/flow_capture/[NAME] 目录下
    - 文件名格式: [NAME]_YYYYMMDD_HHMMSS_[DURATION]_[CONTAINER].pcap
    - 会自动创建输出目录（如果不存在）
    - 网络接口名只能包含字母、数字、下划线、连字符和点号

EOF
}

# 参数解析
while getopts "t:n:c:i:h" opt; do
    case $opt in
        t)
            DURATION="$OPTARG"
            # 验证时间参数是否为正整数
            if ! [[ "$DURATION" =~ ^[1-9][0-9]*$ ]]; then
                echo "错误: 持续时间必须是正整数" >&2
                exit 1
            fi
            ;;
        n)
            DOCKER_FILENAME="$OPTARG"
            # 验证文件名是否合法
            if ! [[ "$DOCKER_FILENAME" =~ ^[a-zA-Z0-9_-]+$ ]]; then
                echo "错误: 文件名只能包含字母、数字、下划线和连字符" >&2
                exit 1
            fi
            ;;
        c)
            CONTAINER_NAME="$OPTARG"
            # 验证容器名是否合法
            if ! [[ "$CONTAINER_NAME" =~ ^[a-zA-Z0-9_-]+$ ]]; then
                echo "错误: 容器名只能包含字母、数字、下划线和连字符" >&2
                exit 1
            fi
            ;;
        i)
            INTERFERENCE="$OPTARG"
            # 验证网络接口名是否合法
            if ! [[ "$INTERFERENCE" =~ ^[a-zA-Z0-9._-]+$ ]]; then
                echo "错误: 网络接口名只能包含字母、数字、下划线、连字符和点号" >&2
                exit 1
            fi
            ;;
        h)
            show_help
            exit 0
            ;;
        \?)
            echo "错误: 无效选项 -$OPTARG" >&2
            echo "使用 -h 查看帮助信息" >&2
            exit 1
            ;;
        :)
            echo "错误: 选项 -$OPTARG 需要参数" >&2
            echo "使用 -h 查看帮助信息" >&2
            exit 1
            ;;
    esac
done

# 移除已处理的选项参数
shift $((OPTIND-1))

# 检查是否有多余的位置参数
if [ $# -gt 0 ]; then
    echo "错误: 不支持位置参数，请使用选项参数" >&2
    echo "使用 -h 查看帮助信息" >&2
    exit 1
fi

# 计算睡眠时间
DURATION_SLEEP=$(( DURATION + 1 ))

# 获取当前时间戳
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# 设置文件名和输出目录
if [ -n "$DOCKER_FILENAME" ]; then
    FILENAME="${DOCKER_FILENAME}_${TIMESTAMP}_${DURATION}_${CONTAINER_NAME}.pcap"
    OUT_DIR="/tmp/flow_capture/"
    OUT_FOLDER="$OUT_DIR$DOCKER_FILENAME"
else
    FILENAME="${TIMESTAMP}_${DURATION}_${CONTAINER_NAME}.pcap"
    OUT_DIR="/tmp/flow_capture/"
    OUT_FOLDER="${OUT_DIR}default"
fi

# 创建输出目录（如果不存在）
mkdir -p "$OUT_FOLDER"

# 显示配置信息
echo "========== 抓包配置 =========="
echo "容器名称: $CONTAINER_NAME"
echo "网络接口: $INTERFERENCE"
echo "持续时间: $DURATION 秒"
echo "文件名: $FILENAME"
echo "输出目录: $OUT_FOLDER/"
echo "=============================="

# 检查容器是否存在和运行（精确匹配）
echo "检查容器状态..."

# 使用docker ps的filter参数进行精确匹配
if ! docker ps -f "name=^${CONTAINER_NAME}$" --format "{{.Names}}" | grep -Fxq "$CONTAINER_NAME"; then
    # 如果方法1不支持正则，使用docker inspect作为备选
    if ! docker inspect "$CONTAINER_NAME" >/dev/null 2>&1; then
        echo "错误: 容器 '$CONTAINER_NAME' 不存在" >&2
        echo "可用的容器:"
        docker ps -a --format "table {{.Names}}\t{{.Status}}"
        exit 1
    fi
    
    # 检查容器是否正在运行
    CONTAINER_STATE=$(docker inspect "$CONTAINER_NAME" --format "{{.State.Status}}" 2>/dev/null)
    if [ "$CONTAINER_STATE" != "running" ]; then
        echo "错误: 容器 '$CONTAINER_NAME' 存在但未运行 (状态: $CONTAINER_STATE)" >&2
        echo "当前运行的容器:"
        docker ps --format "table {{.Names}}\t{{.Status}}"
        exit 1
    fi
fi

echo "容器 '$CONTAINER_NAME' 正在运行"

# 检查容器中的网络接口
echo "检查容器中的网络接口..."
# 使用更安全的方式检查网络接口，避免命令注入
if ! docker exec "$CONTAINER_NAME" /bin/sh -c 'ip link show "$1" > /dev/null 2>&1' -- "$INTERFERENCE"; then
    echo "警告: 网络接口 '$INTERFERENCE' 在容器中不存在" >&2
    echo "容器中可用的网络接口:"
    docker exec "$CONTAINER_NAME" /bin/sh -c "ip link show | grep '^[0-9]' | cut -d: -f2 | tr -d ' '"
    echo "继续使用指定的接口，如果不存在tcpdump会报错..."
fi

# 进入容器并运行tcpdump
echo "开始在容器 '$CONTAINER_NAME' 中抓包，持续 $DURATION 秒..."
echo "文件名: $FILENAME"
echo "抓包持续时间：$DURATION, 等待时间: $DURATION_SLEEP"

# 使用更安全的方式执行tcpdump，通过参数传递避免命令注入
docker exec "$CONTAINER_NAME" /bin/sh -c '
    timeout "$1" tcpdump -i "$2" -w "/data/local/tmp/$3" not port 5555
' -- "$DURATION" "$INTERFERENCE" "$FILENAME" &
TCPDUMP_PID=$!

# 等待抓包完成
echo "等待抓包完成..."
wait $TCPDUMP_PID
TCPDUMP_EXIT_CODE=$?

# 额外等待确保文件写入完成
sleep 2

# 检查tcpdump是否成功执行
if [ $TCPDUMP_EXIT_CODE -ne 0 ] && [ $TCPDUMP_EXIT_CODE -ne 124 ]; then
    echo "警告: tcpdump可能未正常结束 (退出码: $TCPDUMP_EXIT_CODE)" >&2
fi

# # 检查文件是否存在
# if ! docker exec "$CONTAINER_NAME" /bin/sh -c 'test -f "/data/local/tmp/$1"' -- "$FILENAME"; then
#     echo "错误: 抓包文件未生成，可能的原因:" >&2
#     echo "  1. 网络接口不存在或无权限" >&2
#     echo "  2. 容器中没有tcpdump命令" >&2
#     echo "  3. /data/local/tmp/ 目录不存在或无写权限" >&2
#     exit 1
# fi

# 把pcap文件拷贝到本地
echo "正在复制文件到本地..."
if docker cp "$CONTAINER_NAME:/data/local/tmp/$FILENAME" "$OUT_FOLDER/$FILENAME"; then
    echo "文件复制成功"
    
    # 清理容器中的临时文件（安全方式）
    if docker exec "$CONTAINER_NAME" rm -f "/data/local/tmp/$FILENAME"; then
        echo "已清理容器中的临时文件"
    else
        echo "警告: 清理容器中的临时文件失败，但不影响主要功能" >&2
    fi
    
    # 显示文件信息
    if [ -f "$OUT_FOLDER/$FILENAME" ]; then
        FILE_SIZE=$(ls -lh "$OUT_FOLDER/$FILENAME" | awk '{print $5}')
        echo "========== 抓包完成 =========="
        echo "文件保存位置: $OUT_FOLDER/$FILENAME"
        echo "文件大小: $FILE_SIZE"
        echo "=============================="
    fi
else
    echo "错误: 文件复制失败" >&2
    exit 1
fi