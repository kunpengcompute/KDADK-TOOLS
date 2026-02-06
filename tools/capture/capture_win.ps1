# 
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# 

param(
    [Alias("d")]
    [int]$Duration = 30,
    
    [Alias("n")]
    [string]$Name = "capture",
    
    [Alias("i")]
    [string]$Interface = "1",
    
    [Alias("f")]
    [string]$Filter = "",
    
    [switch]$Help,
    
    [Alias("l")]
    [switch]$ListInterfaces
)

function Show-Help {
    Write-Host @"
用法: .\capture_win.ps1 [参数]

参数:
    -d, -Duration         抓包持续时间（秒，默认：30）
    -n, -Name             输出文件名前缀（默认：capture）
    -i, -Interface        网络接口编号（默认：1）
    -f, -Filter           BPF过滤条件
    -l, -ListInterfaces   列出所有网络接口
    -Help                 显示帮助信息

示例:
    .\capture_win.ps1 -d 30 -n bilibili
    .\capture_win.ps1 -d 60 -i 2 -f "port 443"
    .\capture_win.ps1 -l
    .\capture_win.ps1 -Help

说明:
    - 文件名只能包含字母、数字、下划线和连字符
    - 网络接口编号必须是有效的接口ID（使用 -l 查看）
"@
}

function Test-TsharkPath {
    try {
        $cmd = Get-Command tshark -ErrorAction Stop
        return $cmd.Source
    }
    catch {
        $paths = @(
            "C:\Program Files\Wireshark\tshark.exe",
            "C:\Program Files (x86)\Wireshark\tshark.exe"
        )
        
        foreach ($path in $paths) {
            if (Test-Path $path) {
                return $path
            }
        }
        return $null
    }
}

function Get-ValidInterfaces {
    param([string]$TsharkPath)
    
    $output = & $TsharkPath -D 2>&1
    $interfaces = @()
    
    foreach ($line in $output) {
        if ($line -match '^(\d+)\.') {
            $interfaces += $matches[1]
        }
    }
    
    return $interfaces
}

function Show-Interfaces {
    param([string]$TsharkPath)
    
    Write-Host "========== 可用的网络接口 =========="
    & $TsharkPath -D
    Write-Host "===================================="
}

# 处理帮助
if ($Help) {
    Show-Help
    exit 0
}

# 检查tshark
$tsharkPath = Test-TsharkPath
if (-not $tsharkPath) {
    Write-Error "错误: 未找到tshark工具，请安装Wireshark"
    exit 1
}

# 处理接口列表
if ($ListInterfaces) {
    Show-Interfaces -TsharkPath $tsharkPath
    exit 0
}

# 验证持续时间
if ($Duration -le 0) {
    Write-Error "错误: 持续时间必须是正整数"
    exit 1
}

# 验证文件名
if ($Name -notmatch '^[a-zA-Z0-9_-]+$') {
    Write-Error "错误: 文件名只能包含字母、数字、下划线和连字符"
    exit 1
}

# 验证网络接口
$validInterfaces = Get-ValidInterfaces -TsharkPath $tsharkPath
if ($validInterfaces.Count -eq 0) {
    Write-Error "错误: 无法获取网络接口列表"
    exit 1
}

if ($Interface -notmatch '^\d+$') {
    Write-Error "错误: 网络接口必须是数字"
    Write-Host "使用 -l 查看可用的网络接口"
    exit 1
}

if ($validInterfaces -notcontains $Interface) {
    Write-Error "错误: 网络接口 '$Interface' 不存在"
    Write-Host "`n可用的网络接口:"
    Show-Interfaces -TsharkPath $tsharkPath
    exit 1
}

# 设置输出
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$filename = "${Name}_${timestamp}.pcap"
$outdir = "C:\flow_capture\$Name"

if (!(Test-Path $outdir)) {
    New-Item -ItemType Directory -Path $outdir -Force | Out-Null
    Write-Host "已创建输出目录: $outdir"
}

$outputfile = "$outdir\$filename"

# 显示配置
Write-Host "========== 抓包配置 =========="
Write-Host "工具路径: $tsharkPath"
Write-Host "网络接口: $Interface"
Write-Host "持续时间: $Duration 秒"
Write-Host "输出文件: $outputfile"
if ($Filter) {
    Write-Host "过滤条件: $Filter"
}
Write-Host "=============================="

# 执行抓包
Write-Host "开始抓包..."
try {
    if ($Filter) {
        & $tsharkPath -i $Interface -a "duration:$Duration" -w $outputfile -f $Filter
    }
    else {
        & $tsharkPath -i $Interface -a "duration:$Duration" -w $outputfile
    }
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "抓包完成"
    }
    else {
        Write-Error "tshark执行失败，退出代码: $LASTEXITCODE"
        exit 1
    }
}
catch {
    Write-Error "抓包过程出错: $_"
    exit 1
}

# 检查结果
if (Test-Path $outputfile) {
    $fileInfo = Get-Item $outputfile
    $sizeKB = [math]::Round($fileInfo.Length / 1KB, 2)
    
    Write-Host "========== 抓包完成 =========="
    Write-Host "文件位置: $outputfile"
    Write-Host "文件大小: $sizeKB KB"
    Write-Host "=============================="
}
else {
    Write-Error "抓包文件未生成"
    exit 1
}