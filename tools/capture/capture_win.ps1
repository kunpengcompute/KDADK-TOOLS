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
    $helpText = "用法: .\capture_tshark.ps1 [参数]`n`n"
    $helpText += "参数:`n"
    $helpText += "    -d, -Duration         抓包持续时间（秒，默认：30）`n"
    $helpText += "    -n, -Name             输出文件名前缀（默认：capture）`n"
    $helpText += "    -i, -Interface        网络接口编号（默认：1）`n"
    $helpText += "    -f, -Filter           BPF过滤条件`n"
    $helpText += "    -l, -ListInterfaces   列出所有网络接口`n"
    $helpText += "    -Help                 显示帮助信息`n`n"
    $helpText += "示例:`n"
    $helpText += "    .\capture_tshark.ps1 -d 30 -n bilibili`n"
    $helpText += "    .\capture_tshark.ps1 -d 60 -i 2 -f `"port 443`"`n"
    $helpText += "    .\capture_tshark.ps1 -l`n"
    $helpText += "    .\capture_tshark.ps1 -Help`n"
    
    Write-Host $helpText
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

function Show-Interfaces {
    $tsharkPath = Test-TsharkPath
    if ($tsharkPath) {
        Write-Host "可用的网络接口:"
        & $tsharkPath -D
    }
    else {
        Write-Error "未找到tshark，请安装Wireshark"
    }
}

# 处理帮助和接口列表
if ($Help) {
    Show-Help
    exit 0
}

if ($ListInterfaces) {
    Show-Interfaces
    exit 0
}

# 检查tshark
$tsharkPath = Test-TsharkPath
if (-not $tsharkPath) {
    Write-Error "错误: 未找到tshark工具，请安装Wireshark"
    exit 1
}

# 参数验证
if ($Duration -le 0) {
    Write-Error "错误: 持续时间必须是正整数"
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

# 执行抓包 - 使用 & 操作符而不是 Invoke-Expression
Write-Host "开始抓包..."
try {
    if ($Filter) {
        Write-Host "执行命令: tshark -i $Interface -a duration:$Duration -w `"$outputfile`" -f `"$Filter`""
        & $tsharkPath -i $Interface -a "duration:$Duration" -w $outputfile -f $Filter
    }
    else {
        Write-Host "执行命令: tshark -i $Interface -a duration:$Duration -w `"$outputfile`""
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