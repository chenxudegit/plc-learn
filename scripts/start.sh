#!/bin/bash

# 工业设备数据模拟系统启动脚本

echo "=== 工业设备数据模拟系统启动脚本 ==="
echo "版本: 1.0.0"
echo ""

# 检查Python环境
echo "检查Python环境..."
if ! command -v python3 &> /dev/null; then
    echo "错误: 未找到Python3，请先安装Python3"
    exit 1
fi

# 检查C++编译环境
echo "检查C++编译环境..."
if ! command -v g++ &> /dev/null; then
    echo "错误: 未找到g++编译器，请先安装C++编译环境"
    exit 1
fi

# 检查CMake
echo "检查CMake..."
if ! command -v cmake &> /dev/null; then
    echo "警告: 未找到CMake，将使用Makefile编译"
    USE_CMAKE=false
else
    USE_CMAKE=true
fi

# 创建必要的目录
echo "创建必要的目录..."
mkdir -p data
mkdir -p logs

# 安装Python依赖
echo "安装Python依赖..."
if [ -f "requirements.txt" ]; then
    pip3 install -r requirements.txt
    if [ $? -ne 0 ]; then
        echo "警告: Python依赖安装失败，尝试使用pip..."
        pip install -r requirements.txt
    fi
else
    echo "警告: 未找到requirements.txt文件"
fi

# 编译C++代码
echo "编译C++代码..."
if [ "$USE_CMAKE" = true ]; then
    echo "使用CMake编译..."
    mkdir -p build
    cd build
    cmake ..
    make
    if [ $? -eq 0 ]; then
        echo "C++代码编译成功"
    else
        echo "错误: C++代码编译失败"
        exit 1
    fi
    cd ..
else
    echo "使用Makefile编译..."
    make clean
    make
    if [ $? -eq 0 ]; then
        echo "C++代码编译成功"
    else
        echo "错误: C++代码编译失败"
        exit 1
    fi
fi

# 启动C++模拟器（后台运行）
echo "启动C++模拟器..."
if [ -f "build/plc-simulator" ]; then
    ./build/plc-simulator > logs/simulator.log 2>&1 &
    SIMULATOR_PID=$!
    echo "C++模拟器已启动 (PID: $SIMULATOR_PID)"
elif [ -f "build/src/cpp/plc-simulator" ]; then
    ./build/src/cpp/plc-simulator > logs/simulator.log 2>&1 &
    SIMULATOR_PID=$!
    echo "C++模拟器已启动 (PID: $SIMULATOR_PID)"
else
    echo "警告: 未找到编译后的可执行文件"
fi

# 等待一下让模拟器启动
sleep 2

# 启动Web服务
echo "启动Web服务..."
cd src/web
python3 app.py > ../../logs/web.log 2>&1 &
WEB_PID=$!
cd ../..
echo "Web服务已启动 (PID: $WEB_PID)"

# 保存进程ID
echo $SIMULATOR_PID > logs/simulator.pid
echo $WEB_PID > logs/web.pid

echo ""
echo "=== 系统启动完成 ==="
echo "C++模拟器 PID: $SIMULATOR_PID"
echo "Web服务 PID: $WEB_PID"
echo ""
echo "访问地址: http://127.0.0.1:8080"
echo "查看日志: tail -f logs/*.log"
echo ""
echo "停止系统: ./scripts/stop.sh"
echo ""

# 等待用户输入
read -p "按回车键停止系统..."


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
