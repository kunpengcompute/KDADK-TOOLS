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
DEFAULT_INTERFACE="eth0"
DEFAULT_DURATION=30
DEFAULT_FILENAME="default"

# 获取第一个运行中的容器作为默认容器
get_first_container() {
    local first_container=$(docker ps --format "{{.Names}}" | head -n 1)
    if [ -z "$first_container" ]; then
        echo "default"
    else
        echo "$first_container"
    fi
}

DEFAULT_CONTAINER_NAME=$(get_first_container)

# 初始化变量
CONTAINER_NAME="$DEFAULT_CONTAINER_NAME"
INTERFACE="$DEFAULT_INTERFACE"
DURATION="$DEFAULT_DURATION"
DOCKER_FILENAME="$DEFAULT_FILENAME"

# 显示帮助信息
show_help() {
    cat << EOF
用法: $0 [选项]

选项:
    -t DURATION     设置抓包持续时间（单位：秒，默认：$DEFAULT_DURATION秒）
    -n NAME         设置输出文件名前缀（默认：$DEFAULT_FILENAME）
    -c CONTAINER    设置容器名称（默认：自动获取第一个运行中的容器）
    -i INTERFACE    设置网络接口（默认：$DEFAULT_INTERFACE）
    -l              列出所有运行中的容器
    -s CONTAINER    显示指定容器的网络接口
    -h              显示此帮助信息

示例:
    $0 -l                                     # 列出所有容器
    $0 -s android_16                          # 查看容器网络接口
    $0                                        # 使用所有默认值
    $0 -t 30 -n bilibili -c android_16        # 完整参数
    $0 -t 60 -n deepseek                      # 抓包60秒，文件名前缀为deepseek

说明:
    - 抓包文件将保存在 /tmp/flow_capture/[NAME] 目录下
    - 文件名格式: [NAME]_YYYYMMDD_HHMMSS_[DURATION]_[CONTAINER].pcap
    - 会自动创建输出目录（如果不存在）
    - 如果未指定容器，将自动使用第一个运行中的容器

EOF
}

# 列出所有容器
list_containers() {
    echo "========== 运行中的容器 =========="
    docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Image}}"
    echo ""
    echo "========== 所有容器 =========="
    docker ps -a --format "table {{.Names}}\t{{.Status}}\t{{.Image}}"
    exit 0
}

# 显示容器网络接口
show_interfaces() {
    local container="$1"
    if ! docker inspect "$container" >/dev/null 2>&1; then
        echo "错误: 容器 '$container' 不存在" >&2
        exit 1
    fi
    
    echo "========== 容器 '$container' 的网络接口 =========="
    docker exec "$container" ip -o link show | awk -F'[: @]' '{print $3}'
    exit 0
}

# 参数解析
while getopts "t:n:c:i:ls:h" opt; do
    case $opt in
        t)
            DURATION="$OPTARG"
            if ! [[ "$DURATION" =~ ^[1-9][0-9]*$ ]]; then
                echo "错误: 持续时间必须是正整数" >&2
                exit 1
            fi
            ;;
        n)
            DOCKER_FILENAME="$OPTARG"
            if ! [[ "$DOCKER_FILENAME" =~ ^[a-zA-Z0-9_-]+$ ]]; then
                echo "错误: 文件名只能包含字母、数字、下划线和连字符" >&2
                exit 1
            fi
            ;;
        c)
            CONTAINER_NAME="$OPTARG"
            if ! [[ "$CONTAINER_NAME" =~ ^[a-zA-Z0-9_-]+$ ]]; then
                echo "错误: 容器名只能包含字母、数字、下划线和连字符" >&2
                exit 1
            fi
            ;;
        i)
            INTERFACE="$OPTARG"
            if ! [[ "$INTERFACE" =~ ^[a-zA-Z0-9._-]+$ ]]; then
                echo "错误: 网络接口名只能包含字母、数字、下划线、连字符和点号" >&2
                exit 1
            fi
            ;;
        l)
            list_containers
            ;;
        s)
            show_interfaces "$OPTARG"
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

shift $((OPTIND-1))

if [ $# -gt 0 ]; then
    echo "错误: 不支持位置参数，请使用选项参数" >&2
    echo "使用 -h 查看帮助信息" >&2
    exit 1
fi

# 检查容器名称是否为default（说明没有运行中的容器）
if [ "$CONTAINER_NAME" = "default" ]; then
    echo "错误: 没有找到运行中的容器，请使用 -c 指定容器名称" >&2
    echo "使用 -l 查看可用容器" >&2
    exit 1
fi

# 计算睡眠时间
DURATION_SLEEP=$(( DURATION + 1 ))

# 获取当前时间戳
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# 设置文件名和输出目录
FILENAME="${DOCKER_FILENAME}_${TIMESTAMP}_${DURATION}_${CONTAINER_NAME}.pcap"
OUT_DIR="/tmp/flow_capture/"
OUT_FOLDER="$OUT_DIR$DOCKER_FILENAME"

# 创建输出目录
mkdir -p "$OUT_FOLDER"

# 检查容器是否存在和运行
echo "检查容器状态..."
if ! docker inspect "$CONTAINER_NAME" >/dev/null 2>&1; then
    echo "错误: 容器 '$CONTAINER_NAME' 不存在" >&2
    echo "使用 -l 查看可用容器" >&2
    exit 1
fi

CONTAINER_STATE=$(docker inspect "$CONTAINER_NAME" --format "{{.State.Status}}" 2>/dev/null)
if [ "$CONTAINER_STATE" != "running" ]; then
    echo "错误: 容器 '$CONTAINER_NAME' 存在但未运行 (状态: $CONTAINER_STATE)" >&2
    exit 1
fi

echo "容器 '$CONTAINER_NAME' 正在运行"

# 检查网络接口
echo "检查网络接口..."
if ! docker exec "$CONTAINER_NAME" ip link show "$INTERFACE" >/dev/null 2>&1; then
    echo "警告: 网络接口 '$INTERFACE' 在容器中不存在" >&2
    echo "使用 -s $CONTAINER_NAME 查看可用接口" >&2
    echo "继续使用指定的接口，如果不存在tcpdump会报错..."
fi

# 显示配置信息
echo "========== 抓包配置 =========="
echo "容器名称: $CONTAINER_NAME"
echo "网络接口: $INTERFACE"
echo "持续时间: $DURATION 秒"
echo "文件名: $FILENAME"
echo "输出目录: $OUT_FOLDER/"
echo "=============================="

# 开始抓包
echo "开始抓包..."
docker exec "$CONTAINER_NAME" /bin/sh -c '
    timeout "$1" tcpdump -i "$2" -w "/tmp/$3" not port 5555
' -- "$DURATION" "$INTERFACE" "$FILENAME" &
TCPDUMP_PID=$!

echo "等待抓包完成..."
wait $TCPDUMP_PID
TCPDUMP_EXIT_CODE=$?

sleep 2

if [ $TCPDUMP_EXIT_CODE -ne 0 ] && [ $TCPDUMP_EXIT_CODE -ne 124 ]; then
    echo "警告: tcpdump可能未正常结束 (退出码: $TCPDUMP_EXIT_CODE)" >&2
fi

# 复制文件到本地
echo "正在复制文件到本地..."
if docker exec "$CONTAINER_NAME" tar -cf - -C /tmp "$FILENAME" | tar -xf - -C "$OUT_FOLDER/"; then
    echo "文件复制成功"
    
    docker exec "$CONTAINER_NAME" rm -f "/tmp/$FILENAME" 2>/dev/null
    
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