/**
 * @file socket_device.cpp
 * @brief Socket设备实现文件
 * @details 实现了Socket通信的工业设备模拟，包括服务器管理、客户端连接和数据广播
 * @author 工业设备数据模拟系统开发团队
 * @date 2025
 * @version 1.0.0
 */

#include "../../src/socket/socket_device.h"
#include "../../include/common.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
//#include <json/json.h>  // 暂时注释掉，稍后添加

namespace plc {

/**
 * @brief Socket设备构造函数
 * @details 初始化Socket设备的基本属性和自定义数据
 * @param config DeviceConfig 设备配置信息
 * 
 * 初始化内容：
 * - 继承基类Device的配置
 * - 初始化服务器Socket描述符为-1（无效）
 * - 设置计数器初始值为0
 * - 设置状态初始值为"正常"
 * - 记录最后更新时间
 * - 设置更新间隔
 * - 初始化自定义数据
 */
SocketDevice::SocketDevice(const DeviceConfig& config)
    : Device(config)                           // 调用基类构造函数
    , serverSocket_(-1)                        // 初始化服务器Socket描述符为无效值
    , counterValue_(0)                         // 计数器初始值0
    , statusValue_("正常")                     // 状态初始值"正常"
    , lastUpdate_(Utils::getCurrentTime())     // 记录当前时间作为最后更新时间
    , updateInterval_(config.updateInterval) { // 设置更新间隔
    
    std::cout << "Socket设备 " << config.name << " 已创建" << std::endl;
    initializeCustomData();  // 初始化自定义数据
}

/**
 * @brief Socket设备析构函数
 * @details 确保设备在销毁前正确清理资源
 */
SocketDevice::~SocketDevice() {
    cleanup();  // 调用清理函数
}

/**
 * @brief 初始化Socket设备
 * @details 执行Socket设备的初始化操作
 * @return ErrorCode 初始化结果错误码
 * 
 * 初始化流程：
 * 1. 启动Socket服务器
 * 2. 返回初始化结果
 */
ErrorCode SocketDevice::initialize() {
    try {
        std::cout << "初始化Socket设备 " << config_.name << "..." << std::endl;
        
        // 启动Socket服务器
        if (!startSocketServer()) {
            std::cerr << "Socket服务器启动失败" << std::endl;
            return ErrorCode::CONNECTION_FAILED;
        }
        
        std::cout << "Socket设备 " << config_.name << " 初始化成功" << std::endl;
        return ErrorCode::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "Socket设备初始化异常: " << e.what() << std::endl;
        return ErrorCode::UNKNOWN_ERROR;
    }
}

/**
 * @brief 清理Socket设备资源
 * @details 停止Socket服务器并清理相关资源
 * @return ErrorCode 清理结果错误码
 */
ErrorCode SocketDevice::cleanup() {
    try {
        stopSocketServer();  // 停止Socket服务器
        std::cout << "Socket设备 " << config_.name << " 清理完成" << std::endl;
        return ErrorCode::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "Socket设备清理异常: " << e.what() << std::endl;
        return ErrorCode::UNKNOWN_ERROR;
    }
}

/**
 * @brief Socket设备运行循环
 * @details 设备的主要运行循环，负责数据生成、客户端连接管理和数据广播
 * 
 * 运行循环流程：
 * 1. 启动客户端连接处理线程
 * 2. 生成模拟数据
 * 3. 广播数据到所有连接的客户端
 * 4. 等待下次更新间隔
 * 5. 异常处理和状态更新
 * 6. 等待连接处理线程结束
 */
void SocketDevice::runLoop() {
    std::cout << "Socket设备 " << config_.name << " 运行循环启动" << std::endl;
    
    // 启动客户端连接处理线程（独立线程处理连接）
    std::thread acceptThread(&SocketDevice::acceptConnections, this);
    
    while (running_) {
        try {
            // 第一步：生成模拟数据
            generateSimulatedData();
            
            // 第二步：广播数据到所有连接的客户端
            broadcastGeneratedData();
            
            // 第三步：等待下次更新
            std::this_thread::sleep_for(updateInterval_);
            
        } catch (const std::exception& e) {
            // 异常处理：记录错误并更新设备状态
            std::cerr << "Socket设备运行异常: " << e.what() << std::endl;
            updateStatus(DeviceStatus::ERROR);
            std::this_thread::sleep_for(Duration(1000));
        }
    }
    
    // 等待连接处理线程结束
    if (acceptThread.joinable()) {
        acceptThread.join();
    }
    
    std::cout << "Socket设备 " << config_.name << " 运行循环已停止" << std::endl;
}

/**
 * @brief 初始化自定义数据
 * @details 创建和配置设备的自定义数据项
 * 
 * 自定义数据内容：
 * - 系统启动完成
 * - 传感器校准中
 * - 数据采集正常
 * - 网络连接稳定
 * - 设备运行正常
 */
void SocketDevice::initializeCustomData() {
    customData_ = {
        "系统启动完成",
        "传感器校准中",
        "数据采集正常",
        "网络连接稳定",
        "设备运行正常"
    };
}

bool SocketDevice::startSocketServer() {
    // 创建Socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "创建Socket失败" << std::endl;
        return false;
    }
    
    // 设置Socket选项
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "设置Socket选项失败" << std::endl;
        close(serverSocket_);
        return false;
    }
    
    // 绑定地址和端口
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(config_.address.c_str());
    address.sin_port = htons(config_.port);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "绑定Socket失败: " << config_.address << ":" << config_.port << std::endl;
        close(serverSocket_);
        return false;
    }
    
    // 开始监听
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Socket监听失败" << std::endl;
        close(serverSocket_);
        return false;
    }
    
    std::cout << "Socket服务器已启动，监听地址: " << config_.address << ":" << config_.port << std::endl;
    return true;
}

void SocketDevice::stopSocketServer() {
    // 关闭所有客户端连接
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (int clientSocket : clientSockets_) {
            close(clientSocket);
        }
        clientSockets_.clear();
    }
    
    // 关闭服务器Socket
    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
    
    std::cout << "Socket服务器已停止" << std::endl;
}

void SocketDevice::acceptConnections() {
    std::cout << "Socket连接接受线程已启动" << std::endl;
    
    while (running_ && serverSocket_ >= 0) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        
        // 接受客户端连接
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddress, &clientAddressLength);
        
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "接受客户端连接失败" << std::endl;
            }
            continue;
        }
        
        // 获取客户端地址
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddress.sin_port);
        
        std::cout << "新客户端连接: " << clientIP << ":" << clientPort << std::endl;
        
        // 添加到客户端列表
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            clientSockets_.push_back(clientSocket);
        }
        
        // 启动客户端处理线程
        std::thread clientThread(&SocketDevice::handleClient, this, clientSocket);
        clientThread.detach(); // 分离线程，让其独立运行
    }
    
    std::cout << "Socket连接接受线程已停止" << std::endl;
}

void SocketDevice::handleClient(int clientSocket) {
    std::cout << "客户端处理线程启动，Socket: " << clientSocket << std::endl;
    
    char buffer[1024];
    
    while (running_) {
        // 接收客户端数据
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
        
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "收到客户端数据: " << buffer << std::endl;
            
            // 处理客户端请求
            handleClientRequest(clientSocket, String(buffer));
            
        } else if (bytesReceived == 0) {
            // 客户端断开连接
            std::cout << "客户端断开连接，Socket: " << clientSocket << std::endl;
            break;
        } else {
            // 没有数据或错误，继续等待
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // 清理客户端连接
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = std::find(clientSockets_.begin(), clientSockets_.end(), clientSocket);
        if (it != clientSockets_.end()) {
            clientSockets_.erase(it);
        }
    }
    
    close(clientSocket);
    std::cout << "客户端处理线程结束，Socket: " << clientSocket << std::endl;
}

void SocketDevice::handleClientRequest(int clientSocket, const String& request) {
    try {
        // 简单文本协议处理
        if (request == "DATA") {
            sendCurrentDataToClient(clientSocket);
        } else if (request == "STATUS") {
            sendStatusToClient(clientSocket);
        } else {
            sendErrorToClient(clientSocket, "未知请求: " + request);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "处理客户端请求异常: " << e.what() << std::endl;
        sendErrorToClient(clientSocket, "服务器内部错误");
    }
}

void SocketDevice::sendCurrentDataToClient(int clientSocket) {
    // 使用简单的文本格式
    String response = "DATA|" + Utils::getCurrentTimeString() + "|" + config_.name + "|" + 
                     std::to_string(counterValue_) + "|" + statusValue_ + "\n";
    
    send(clientSocket, response.c_str(), response.length(), 0);
}

void SocketDevice::sendStatusToClient(int clientSocket) {
    String response = "STATUS|" + Utils::getCurrentTimeString() + "|" + config_.name + "|" + 
                     statusValue_ + "|" + std::to_string(counterValue_) + "\n";
    
    send(clientSocket, response.c_str(), response.length(), 0);
}

void SocketDevice::sendErrorToClient(int clientSocket, const String& error) {
    String response = "ERROR|" + Utils::getCurrentTimeString() + "|" + error + "\n";
    
    send(clientSocket, response.c_str(), response.length(), 0);
}

void SocketDevice::generateSimulatedData() {
    auto now = Utils::getCurrentTime();
    
    // 模拟计数器
    simulateCounter();
    
    // 模拟状态
    simulateStatus();
    
    // 模拟自定义数据
    simulateCustomData();
    
    lastUpdate_ = now;
}

void SocketDevice::simulateCounter() {
    counterValue_++;
    
    // 偶尔重置计数器
    if (counterValue_ > 10000) {
        counterValue_ = 0;
    }
}

void SocketDevice::simulateStatus() {
    static int statusCounter = 0;
    statusCounter++;
    
    // 每200次更新改变一次状态
    if (statusCounter % 200 == 0) {
        std::vector<String> statusOptions = {"正常", "警告", "维护", "忙碌"};
        int index = statusCounter / 200 % statusOptions.size();
        statusValue_ = statusOptions[index];
    }
}

void SocketDevice::simulateCustomData() {
    static int dataCounter = 0;
    dataCounter++;
    
    // 每50次更新改变一次自定义数据
    if (dataCounter % 50 == 0) {
        // 随机选择一个自定义数据项进行更新
        int index = dataCounter / 50 % customData_.size();
        customData_[index] = "更新时间: " + Utils::getCurrentTimeString();
    }
}

bool SocketDevice::sendMessage(const SocketMessage& message) {
    try {
        String textMessage = "MSG|" + message.type + "|" + message.data + "|" + Utils::getCurrentTimeString() + "\n";
        broadcastToClients(textMessage);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "发送Socket消息异常: " << e.what() << std::endl;
        return false;
    }
}

void SocketDevice::broadcastData(const DataPoint& data) {
    try {
        String textData = "BROADCAST|" + config_.name + "|" + Utils::dataTypeToString(data.type) + "|" + 
                         std::to_string(data.value) + "|" + data.unit + "|" + Utils::getCurrentTimeString() + "\n";
        
        broadcastToClients(textData);
        
    } catch (const std::exception& e) {
        std::cerr << "广播数据异常: " << e.what() << std::endl;
    }
}

void SocketDevice::broadcastToClients(const String& message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    // 向所有连接的客户端发送消息
    auto it = clientSockets_.begin();
    while (it != clientSockets_.end()) {
        int clientSocket = *it;
        
        ssize_t bytesSent = send(clientSocket, message.c_str(), message.length(), MSG_NOSIGNAL);
        
        if (bytesSent < 0) {
            // 发送失败，可能客户端已断开连接
            std::cout << "客户端连接已断开，Socket: " << clientSocket << std::endl;
            close(clientSocket);
            it = clientSockets_.erase(it);
        } else {
            ++it;
        }
    }
}

void SocketDevice::broadcastGeneratedData() {
    // 创建数据点并发送到回调
    DataPoint counterData(DataType::CUSTOM, static_cast<double>(counterValue_), "计数", config_.name);
    sendData(counterData);
    
    DataPoint statusData(DataType::STATUS, 0.0, statusValue_, config_.name);
    sendData(statusData);
    
    // 广播到Socket客户端
    broadcastData(counterData);
}

} // namespace plc
