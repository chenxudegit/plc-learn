/**
 * @file device.cpp
 * @brief 设备基类实现文件
 * @details 实现了设备管理的基础功能，包括启动、停止、状态管理、线程管理等
 * @author 工业设备数据模拟系统开发团队
 * @date 2025
 * @version 1.0.0
 */

#include "../../include/device.h"
#include "../../include/common.h"
#include <iostream>
#include <chrono>

namespace plc {

/**
 * @brief 设备构造函数
 * @details 初始化设备的基本属性，设置默认状态为离线
 * @param config DeviceConfig 设备配置信息
 * 
 * 初始化内容：
 * - 保存设备配置
 * - 设置初始状态为离线
 * - 设置运行标志为false
 */
Device::Device(const DeviceConfig& config)
    : config_(config)                    // 保存设备配置
    , status_(DeviceStatus::OFFLINE)     // 初始状态设为离线
    , running_(false) {                  // 初始运行标志设为false
    std::cout << "设备 " << config_.name << " 已创建" << std::endl;
}

/**
 * @brief 设备析构函数
 * @details 确保设备在销毁前正确停止和清理资源
 */
Device::~Device() {
    stop();      // 停止设备运行
    // 注意：不在析构函数中调用cleanup()，因为它是纯虚函数
    // 子类应该在stop()方法中完成清理工作
}

/**
 * @brief 启动设备
 * @details 执行设备启动流程：初始化、启动工作线程、更新状态
 * @return ErrorCode 启动结果错误码
 * 
 * 启动流程：
 * 1. 检查设备是否已在运行
 * 2. 执行设备初始化
 * 3. 启动工作线程
 * 4. 更新设备状态为在线
 * 5. 设置运行标志
 */
ErrorCode Device::start() {
    // 检查设备是否已在运行
    if (running_) {
        std::cout << "设备 " << config_.name << " 已在运行中" << std::endl;
        return ErrorCode::SUCCESS;
    }
    
    try {
        std::cout << "启动设备 " << config_.name << "..." << std::endl;
        
        // 第一步：初始化设备
        ErrorCode initResult = initialize();
        if (initResult != ErrorCode::SUCCESS) {
            std::cerr << "设备 " << config_.name << " 初始化失败" << std::endl;
            return initResult;
        }
        
        // 第二步：启动工作线程
        startThread();
        
        // 第三步：更新设备状态为在线
        updateStatus(DeviceStatus::ONLINE);
        running_ = true;  // 设置运行标志
        
        std::cout << "设备 " << config_.name << " 启动成功" << std::endl;
        return ErrorCode::SUCCESS;
        
    } catch (const std::exception& e) {
        // 异常处理：记录错误并更新状态
        std::cerr << "设备 " << config_.name << " 启动失败: " << e.what() << std::endl;
        updateStatus(DeviceStatus::ERROR);
        return ErrorCode::UNKNOWN_ERROR;
    }
}

/**
 * @brief 停止设备
 * @details 执行设备停止流程：停止线程、清理资源、更新状态
 * @return ErrorCode 停止结果错误码
 * 
 * 停止流程：
 * 1. 检查设备是否在运行
 * 2. 停止工作线程
 * 3. 执行资源清理
 * 4. 更新设备状态为离线
 * 5. 清除运行标志
 */
ErrorCode Device::stop() {
    // 检查设备是否在运行
    if (!running_) {
        return ErrorCode::SUCCESS;
    }
    
    try {
        std::cout << "停止设备 " << config_.name << "..." << std::endl;
        
        // 第一步：停止工作线程
        stopThread();
        
        // 第二步：清理设备资源
        ErrorCode cleanupResult = cleanup();
        if (cleanupResult != ErrorCode::SUCCESS) {
            std::cerr << "设备 " << config_.name << " 清理失败" << std::endl;
        }
        
        // 第三步：更新设备状态为离线
        updateStatus(DeviceStatus::OFFLINE);
        running_ = false;  // 清除运行标志
        
        std::cout << "设备 " << config_.name << " 已停止" << std::endl;
        return ErrorCode::SUCCESS;
        
    } catch (const std::exception& e) {
        // 异常处理：记录错误信息
        std::cerr << "设备 " << config_.name << " 停止失败: " << e.what() << std::endl;
        return ErrorCode::UNKNOWN_ERROR;
    }
}

/**
 * @brief 重置设备
 * @details 执行设备重置流程：停止、等待、重新启动
 * @return ErrorCode 重置结果错误码
 * 
 * 重置流程：
 * 1. 停止当前运行的设备
 * 2. 等待一段时间确保资源释放
 * 3. 重新启动设备
 */
ErrorCode Device::reset() {
    try {
        std::cout << "重置设备 " << config_.name << "..." << std::endl;
        
        // 第一步：停止设备
        stop();
        
        // 第二步：等待资源释放（1秒）
        std::this_thread::sleep_for(Duration(1000));
        
        // 第三步：重新启动设备
        return start();
        
    } catch (const std::exception& e) {
        std::cerr << "设备 " << config_.name << " 重置失败: " << e.what() << std::endl;
        return ErrorCode::UNKNOWN_ERROR;
    }
}

/**
 * @brief 更新设备配置
 * @details 更新设备配置信息，支持运行时配置更新
 * @param config DeviceConfig 新的设备配置
 * 
 * 配置更新流程：
 * 1. 保存旧配置（可用于回滚）
 * 2. 应用新配置
 * 3. 如果设备正在运行，考虑热更新逻辑
 */
void Device::updateConfig(const DeviceConfig& config) {
    try {
        std::cout << "更新设备 " << config_.name << " 配置..." << std::endl;
        
        // 保存旧配置，用于可能的回滚操作
        DeviceConfig oldConfig = config_;
        
        // 应用新配置
        config_ = config;
        
        // 如果设备正在运行，可能需要重新初始化
        if (running_) {
            std::cout << "设备正在运行，应用新配置..." << std::endl;
            // TODO: 这里可以添加配置热更新的逻辑
            // 例如：重新初始化某些组件、更新通信参数等
        }
        
        std::cout << "设备 " << config_.name << " 配置更新成功" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "设备 " << config_.name << " 配置更新失败: " << e.what() << std::endl;
    }
}

/**
 * @brief 发送数据点
 * @details 处理数据点，调用回调函数并可能存储到数据库
 * @param data DataPoint 要发送的数据点
 * 
 * 数据处理流程：
 * 1. 调用数据回调函数（如果已设置）
 * 2. 可以扩展添加数据存储逻辑
 */
void Device::sendData(const DataPoint& data) {
    try {
        // 调用数据回调函数，用于数据转发或处理
        if (dataCallback_) {
            dataCallback_(data);
        }
        
        // TODO: 这里可以添加数据发送到数据库或其他存储的逻辑
        // 例如：写入SQLite数据库、发送到消息队列等
        
    } catch (const std::exception& e) {
        std::cerr << "设备 " << config_.name << " 发送数据失败: " << e.what() << std::endl;
    }
}

/**
 * @brief 更新设备状态
 * @details 更新设备状态并触发状态变化回调
 * @param newStatus DeviceStatus 新的设备状态
 * 
 * 状态更新流程：
 * 1. 检查状态是否发生变化
 * 2. 更新状态值
 * 3. 记录状态变化日志
 * 4. 调用状态回调函数
 */
void Device::updateStatus(DeviceStatus newStatus) {
    // 只在状态真正发生变化时执行更新
    if (status_ != newStatus) {
        DeviceStatus oldStatus = status_;  // 保存旧状态
        status_ = newStatus;               // 更新为新状态
        
        // 记录状态变化日志
        std::cout << "设备 " << config_.name << " 状态从 " 
                  << static_cast<int>(oldStatus) << " 变为 " 
                  << static_cast<int>(newStatus) << std::endl;
        
        // 调用状态回调函数，通知状态变化
        if (statusCallback_) {
            statusCallback_(newStatus);
        }
    }
}

/**
 * @brief 启动工作线程
 * @details 创建并启动设备的工作线程
 * 
 * 线程管理：
 * - 如果已有线程在运行，先等待其结束
 * - 创建新的工作线程执行 workerFunction
 */
void Device::startThread() {
    // 确保之前的线程已经完全结束
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    // 创建新的工作线程，执行 workerFunction 成员函数
    workerThread_ = std::thread(&Device::workerFunction, this);
}

/**
 * @brief 停止工作线程
 * @details 等待工作线程正常结束
 * 
 * 线程停止：
 * - 等待线程自然结束（join）
 * - 确保资源正确释放
 */
void Device::stopThread() {
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

/**
 * @brief 工作线程主函数
 * @details 工作线程的执行入口，负责设备的运行循环
 * 
 * 工作流程：
 * 1. 记录线程启动日志
 * 2. 执行设备特定的运行循环
 * 3. 异常处理和状态更新
 * 4. 记录线程结束日志
 */
void Device::workerFunction() {
    std::cout << "设备 " << config_.name << " 工作线程已启动" << std::endl;
    
    try {
        // 执行设备特定的运行循环
        runLoop();
    } catch (const std::exception& e) {
        // 异常处理：记录错误并更新设备状态为错误
        std::cerr << "设备 " << config_.name << " 工作线程异常: " << e.what() << std::endl;
        updateStatus(DeviceStatus::ERROR);
    }
    
    std::cout << "设备 " << config_.name << " 工作线程已停止" << std::endl;
}

/**
 * @brief 更新循环函数
 * @details 默认的更新循环，子类可以重写以添加特定逻辑
 * 
 * 当前实现：
 * - 每30秒输出一次设备状态信息
 * - 可以扩展添加定期状态检查、数据采集等逻辑
 */
void Device::updateLoop() {
    // 默认的更新循环，子类可以重写
    // 这里可以添加定期状态检查、数据采集等逻辑
    
    static int counter = 0;
    if (++counter % 30 == 0) { // 每30秒输出一次状态
        std::cout << "设备 " << config_.name << " 状态: " 
                  << static_cast<int>(status_) 
                  << " (运行中: " << (running_ ? "是" : "否") << ")" << std::endl;
    }
}

/**
 * @brief 设备初始化（虚函数）
 * @details 默认实现：总是返回成功
 * @return ErrorCode 初始化结果
 * 
 * 说明：
 * - 子类应该重写此函数以实现具体的初始化逻辑
 * - 例如：建立网络连接、初始化硬件接口等
 */
ErrorCode Device::initialize() {
    // 默认实现：总是成功
    return ErrorCode::SUCCESS;
}

/**
 * @brief 设备清理（虚函数）
 * @details 默认实现：总是返回成功
 * @return ErrorCode 清理结果
 * 
 * 说明：
 * - 子类应该重写此函数以实现具体的清理逻辑
 * - 例如：关闭网络连接、释放硬件资源等
 */
ErrorCode Device::cleanup() {
    // 默认实现：总是成功
    return ErrorCode::SUCCESS;
}

/**
 * @brief 设备运行循环（虚函数）
 * @details 默认的运行循环实现，子类可以重写
 * 
 * 运行循环流程：
 * 1. 检查运行标志
 * 2. 执行更新循环
 * 3. 等待下次更新间隔
 * 4. 异常处理和恢复
 */
void Device::runLoop() {
    // 默认实现：简单的循环
    while (running_) {
        try {
            // 执行更新循环
            updateLoop();
            
            // 等待下次更新（使用配置中的更新间隔）
            std::this_thread::sleep_for(config_.updateInterval);
            
        } catch (const std::exception& e) {
            // 异常处理：记录错误并等待1秒后继续
            std::cerr << "设备 " << config_.name << " 运行循环异常: " << e.what() << std::endl;
            std::this_thread::sleep_for(Duration(1000));
        }
    }
}

} // namespace plc
