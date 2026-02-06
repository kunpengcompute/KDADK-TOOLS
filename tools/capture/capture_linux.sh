# 
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# 

#!/bin/bash

# capture_linux.sh - Linux网络流量抓包脚本

# 默认参数
DURATION=30
NAME="capture"
INTERFACE=""
FILTER=""
OUTPUT_DIR=""
HELP=false
LIST_INTERFACES=false

# 显示帮助信息
show_help() {
    cat << EOF
用法: ./capture_linux.sh [参数]

参数:
    -d, --duration         抓包持续时间（秒，默认：30）
    -n, --name             输出文件名前缀（默认：capture）
    -i, --interface        网络接口名称（如：eth0, wlan0等）
    -f, --filter           BPF过滤条件（如："port 443"）
    -o, --output-dir       输出目录（默认：/tmp）
    -l, --list-interfaces  列出所有网络接口
    -h, --help             显示帮助信息

示例:
    ./capture_linux.sh -d 30 -n bilibili
    ./capture_linux.sh -d 60 -i eth0 -f "port 443"
    ./capture_linux.sh -o /home/user/captures -n test
    ./capture_linux.sh -l
    ./capture_linux.sh -h

注意:
    - 需要root权限或sudo权限运行
    - 确保系统已安装tcpdump工具
    - 输出目录必须存在且可写
EOF
}

# 检查tcpdump是否可用
test_tcpdump() {
    if command -v tcpdump >/dev/null 2>&1; then
        return 0
    else
        echo "错误: 未找到tcpdump工具，请先安装"
        echo "Ubuntu/Debian: sudo apt-get install tcpdump"
        echo "CentOS/RHEL: sudo yum install tcpdump"
        echo "Fedora: sudo dnf install tcpdump"
        return 1
    fi
}

get_interfaces_list() {
    # 优先使用 /sys/class/net（最可靠）
    if [ -d /sys/class/net ]; then
        ls /sys/class/net
    elif command -v ip >/dev/null 2>&1; then
        ip link show | grep -E "^[0-9]+:" | cut -d: -f2 | sed 's/^ *//' | awk '{print $1}'
    elif command -v ifconfig >/dev/null 2>&1; then
        ifconfig -a | grep -E "^[a-zA-Z0-9]+" | cut -d: -f1 | awk '{print $1}'
    fi
}

# 显示所有网络接口
show_interfaces() {
    echo "可用的网络接口:"
    get_interfaces_list | while read -r iface; do
        local status=""
        if [ -f "/sys/class/net/$iface/operstate" ]; then
            status=" [$(cat /sys/class/net/$iface/operstate 2>/dev/null)]"
        fi
        echo "  $iface$status"
    done
}

interface_exists() {
    local iface="$1"
    [ -d "/sys/class/net/$iface" ] || \
    ip link show "$iface" >/dev/null 2>&1 || \
    ifconfig "$iface" >/dev/null 2>&1
}

# 获取默认网络接口
get_default_interface() {
    if command -v ip >/dev/null 2>&1; then
        ip route | grep default | head -1 | sed 's/.*dev \([^ ]*\).*/\1/'
    else
        # 返回第一个非 lo 接口
        for iface in /sys/class/net/*; do
            iface=$(basename "$iface")
            [ "$iface" != "lo" ] && echo "$iface" && return
        done
        echo "lo"
    fi
}

# 验证接口名称（防止命令注入）
validate_interface() {
    local iface="$1"
    
    # 验证格式（防止命令注入）
    if ! [[ "$iface" =~ ^[a-zA-Z0-9._:-]+$ ]]; then
        echo "错误: 接口名称 '$iface' 包含非法字符" >&2
        echo "接口名称只能包含字母、数字、点、冒号、连字符和下划线" >&2
        return 1
    fi
    
    # 检查接口是否存在
    if ! interface_exists "$iface"; then
        echo "错误: 网络接口 '$iface' 不存在" >&2
        echo "" >&2
        show_interfaces >&2
        return 1
    fi
    
    return 0
}

# 验证BPF过滤器（基本验证）
validate_filter() {
    local filter="$1"
    # 检查是否包含可能的命令注入字符
    if [[ "$filter" =~ [\;\|\&\$\`] ]]; then
        echo "错误: 过滤器包含非法字符: $filter"
        echo "过滤器不能包含 ; | & $ \` 等字符"
        return 1
    fi
    return 0
}

# 验证并创建输出目录
setup_output_dir() {
    local dir="$1"
    
    # 转换相对路径为绝对路径
    dir=$(realpath "$dir" 2>/dev/null || readlink -f "$dir" 2>/dev/null || echo "$dir")
    
    # 如果目录不存在，尝试创建
    if [ ! -d "$dir" ]; then
        if ! mkdir -p "$dir" 2>/dev/null; then
            echo "错误: 无法创建输出目录: $dir"
            return 1
        fi
        echo "已创建输出目录: $dir"
    fi
    
    # 检查目录是否可写
    if [ ! -w "$dir" ]; then
        echo "错误: 输出目录不可写: $dir"
        return 1
    fi
    
    # 为tcpdump设置适当的权限
    if [ "$EUID" -eq 0 ]; then
        # 设置目录权限
        chmod 750 "$dir"
        
        # 如果tcpdump用户存在，设置组权限
        if id tcpdump >/dev/null 2>&1; then
            chown root:tcpdump "$dir" 2>/dev/null || true
            chmod 750 "$dir" 2>/dev/null || true
        fi
        
        echo "已设置目录权限: $dir"
    fi
    
    # 更新OUTPUT_DIR为绝对路径
    OUTPUT_DIR="$dir"
    
    return 0
}

# 参数解析
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--duration)
            DURATION="$2"
            shift 2
            ;;
        -n|--name)
            NAME="$2"
            shift 2
            ;;
        -i|--interface)
            INTERFACE="$2"
            shift 2
            ;;
        -f|--filter)
            FILTER="$2"
            shift 2
            ;;
        -o|--output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -l|--list-interfaces)
            LIST_INTERFACES=true
            shift
            ;;
        -h|--help)
            HELP=true
            shift
            ;;
        *)
            echo "未知参数: $1"
            show_help
            exit 1
            ;;
    esac
done

# 处理帮助和接口列表
if [ "$HELP" = true ]; then
    show_help
    exit 0
fi

if [ "$LIST_INTERFACES" = true ]; then
    show_interfaces
    exit 0
fi

# 检查tcpdump
if ! test_tcpdump; then
    exit 1
fi

# 检查权限
if [ "$EUID" -ne 0 ]; then
    echo "警告: 建议使用root权限或sudo运行此脚本"
    echo "如果遇到权限问题，请使用: sudo $0 $*"
fi

# 参数验证
if ! [[ "$DURATION" =~ ^[0-9]+$ ]] || [ "$DURATION" -le 0 ]; then
    echo "错误: 持续时间必须是正整数"
    exit 1
fi

# 验证文件名前缀（防止路径遍历）
if ! [[ "$NAME" =~ ^[a-zA-Z0-9_-]+$ ]]; then
    echo "错误: 文件名只能包含字母、数字、下划线和连字符" >&2
    exit 1
fi

# 设置默认接口
if [ -z "$INTERFACE" ]; then
    INTERFACE=$(get_default_interface)
    echo "未指定接口，使用默认接口: $INTERFACE"
fi

# 验证接口名称
if ! validate_interface "$INTERFACE"; then
    exit 1
fi

# 验证过滤器
if [ -n "$FILTER" ] && ! validate_filter "$FILTER"; then
    exit 1
fi

# 设置输出目录（默认使用系统临时目录）
if [ -z "$OUTPUT_DIR" ]; then
    OUTPUT_DIR="/tmp"
fi

# 验证并设置输出目录
if ! setup_output_dir "$OUTPUT_DIR"; then
    exit 1
fi

# 设置输出文件
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
FILENAME="${NAME}_${TIMESTAMP}.pcap"
OUTPUTFILE="$OUTPUT_DIR/$FILENAME"

# 显示配置
echo "========== 抓包配置 =========="
echo "工具路径: $(which tcpdump)"
echo "网络接口: $INTERFACE"
echo "持续时间: $DURATION 秒"
echo "输出目录: $OUTPUT_DIR"
echo "输出文件: $OUTPUTFILE"
if [ -n "$FILTER" ]; then
    echo "过滤条件: $FILTER"
fi
echo "=============================="

# 执行抓包
echo "开始抓包..."

# 构建tcpdump命令参数数组（安全方式）
TCPDUMP_ARGS=()
TCPDUMP_ARGS+=("-i" "$INTERFACE")
TCPDUMP_ARGS+=("-w" "$OUTPUTFILE")

# 如果是root用户，添加-Z root参数禁用权限降级
if [ "$EUID" -eq 0 ]; then
    TCPDUMP_ARGS+=("-Z" "root")
fi

# 如果有过滤条件，添加到参数数组
if [ -n "$FILTER" ]; then
    TCPDUMP_ARGS+=("$FILTER")
fi

# 显示执行的命令（用于调试）
echo "执行命令: timeout ${DURATION}s tcpdump ${TCPDUMP_ARGS[*]}"

# 安全执行tcpdump命令
if timeout ${DURATION}s tcpdump "${TCPDUMP_ARGS[@]}"; then
    echo "抓包完成"
else
    TCPDUMP_EXIT_CODE=$?
    # timeout 的退出代码 124 表示超时，这是正常的
    if [ $TCPDUMP_EXIT_CODE -eq 124 ]; then
        echo "抓包完成（达到指定时长）"
    else
        echo "错误: tcpdump执行失败，退出代码: $TCPDUMP_EXIT_CODE"
        exit 1
    fi
fi

# 检查结果
if [ -f "$OUTPUTFILE" ]; then
    FILE_SIZE=$(ls -lh "$OUTPUTFILE" | awk '{print $5}')
    
    echo "========== 抓包完成 =========="
    echo "文件位置: $OUTPUTFILE"
    echo "文件大小: $FILE_SIZE"
    echo "=============================="
    
    # 显示简单的统计信息
    if command -v tcpdump >/dev/null 2>&1; then
        echo ""
        echo "数据包统计:"
        PACKET_COUNT=$(tcpdump -r "$OUTPUTFILE" 2>/dev/null | wc -l)
        echo "总数据包数: $PACKET_COUNT"
    fi
else
    echo "错误: 抓包文件未生成"
    exit 1
fi