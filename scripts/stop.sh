#!/bin/bash

# 工业设备数据模拟系统停止脚本

echo "=== 停止工业设备数据模拟系统 ==="

# 停止C++模拟器
if [ -f "logs/simulator.pid" ]; then
    SIMULATOR_PID=$(cat logs/simulator.pid)
    echo "停止C++模拟器 (PID: $SIMULATOR_PID)..."
    kill $SIMULATOR_PID 2>/dev/null
    rm logs/simulator.pid
    echo "C++模拟器已停止"
else
    echo "未找到C++模拟器进程ID文件"
fi

# 停止Web服务
if [ -f "logs/web.pid" ]; then
    WEB_PID=$(cat logs/web.pid)
    echo "停止Web服务 (PID: $WEB_PID)..."
    kill $WEB_PID 2>/dev/null
    rm logs/web.pid
    echo "Web服务已停止"
else
    echo "未找到Web服务进程ID文件"
fi

# 检查是否还有相关进程在运行
echo "检查剩余进程..."
PIDS=$(ps aux | grep -E "(plc-simulator|app.py)" | grep -v grep | awk '{print $2}')

if [ -n "$PIDS" ]; then
    echo "发现剩余进程，正在强制停止..."
    echo $PIDS | xargs kill -9 2>/dev/null
    echo "所有相关进程已停止"
else
    echo "没有发现相关进程"
fi

echo "系统已完全停止"
