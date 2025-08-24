/**
 * @file data_collector.cpp
 * @brief 数据采集器实现文件
 * @details 负责管理和协调多个设备的数据采集，提供统一的设备管理接口
 * @author 工业设备数据模拟系统开发团队
 * @date 2025
 * @version 1.0.0
 */

#include "../../include/data_collector.h"
#include "../../include/common.h"
#include <iostream>
#include <algorithm>
#include <chrono>

namespace plc {

/**
 * @brief 数据采集器构造函数
 * @details 初始化数据采集器的基本属性
 * 
 * 初始化内容：
 * - 设置运行标志为false
 * - 设置默认采集间隔为1秒
 */
DataCollector::DataCollector() 
    : running_(false)                    // 初始运行标志设为false
    , collectionInterval_(Duration(1000)) { // 默认采集间隔1秒
    std::cout << "数据采集器已创建" << std::endl;
}

/**
 * @brief 数据采集器析构函数
 * @details 确保采集器在销毁前正确停止和清理资源
 */
DataCollector::~DataCollector() {
    stop();      // 停止采集器
    cleanup();   // 清理资源
}

/**
 * @brief 初始化数据采集器
 * @details 执行采集器的初始化操作
 * @return bool 初始化是否成功
 * 
 * 初始化流程：
 * 1. 设置默认采集间隔
 * 2. 可以扩展添加其他初始化逻辑
 */
bool DataCollector::initialize() {
    try {
        std::cout << "初始化数据采集器..." << std::endl;
        
        // 设置默认采集间隔为1秒
        collectionInterval_ = Duration(1000);
        
        std::cout << "数据采集器初始化成功" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "数据采集器初始化失败: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief 启动数据采集器
 * @details 启动采集器并开始数据采集线程
 * @return bool 启动是否成功
 * 
 * 启动流程：
 * 1. 检查是否已在运行
 * 2. 设置运行标志
 * 3. 启动采集线程
 */
bool DataCollector::start() {
    // 检查采集器是否已在运行
    if (running_) {
        std::cout << "数据采集器已在运行中" << std::endl;
        return true;
    }
    
    try {
        std::cout << "启动数据采集器..." << std::endl;
        running_ = true;  // 设置运行标志
        
        // 启动数据采集线程
        startThread();
        
        std::cout << "数据采集器启动成功" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "数据采集器启动失败: " << e.what() << std::endl;
        running_ = false;  // 启动失败时重置运行标志
        return false;
    }
}

/**
 * @brief 停止数据采集器
 * @details 停止采集器并等待采集线程结束
 * @return bool 停止是否成功
 * 
 * 停止流程：
 * 1. 检查是否在运行
 * 2. 清除运行标志
 * 3. 停止采集线程
 */
bool DataCollector::stop() {
    // 检查采集器是否在运行
    if (!running_) {
        return true;
    }
    
    try {
        std::cout << "停止数据采集器..." << std::endl;
        running_ = false;  // 清除运行标志
        
        // 停止数据采集线程
        stopThread();
        
        std::cout << "数据采集器已停止" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "数据采集器停止失败: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief 清理数据采集器资源
 * @details 清理设备列表和其他资源
 * 
 * 清理内容：
 * - 清空设备列表
 * - 可以扩展添加其他资源清理逻辑
 */
void DataCollector::cleanup() {
    try {
        // 清理设备列表，释放所有设备引用
        devices_.clear();
        std::cout << "数据采集器清理完成" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "数据采集器清理失败: " << e.what() << std::endl;
    }
}

/**
 * @brief 添加设备到采集器
 * @details 将设备添加到采集器的管理列表中
 * @param device std::shared_ptr<Device> 要添加的设备智能指针
 * 
 * 添加流程：
 * 1. 验证设备指针有效性
 * 2. 检查设备是否已存在
 * 3. 如果存在则替换，否则添加
 * 4. 如果采集器正在运行，启动新设备
 */
void DataCollector::addDevice(std::shared_ptr<Device> device) {
    // 验证设备指针有效性
    if (!device) {
        std::cerr << "尝试添加空设备" << std::endl;
        return;
    }
    
    try {
        // 检查设备是否已存在（通过设备名称判断）
        auto it = std::find_if(devices_.begin(), devices_.end(),
            [&device](const std::shared_ptr<Device>& d) {
                return d->getName() == device->getName();
            });
        
        // 如果设备已存在，先移除旧设备
        if (it != devices_.end()) {
            std::cout << "设备 " << device->getName() << " 已存在，将被替换" << std::endl;
            devices_.erase(it);
        }
        
        // 添加新设备到列表
        devices_.push_back(device);
        std::cout << "设备 " << device->getName() << " 已添加到采集器" << std::endl;
        
        // 如果采集器正在运行，立即启动新添加的设备
        if (running_) {
            device->start();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "添加设备失败: " << e.what() << std::endl;
    }
}

/**
 * @brief 从采集器移除设备
 * @details 根据设备名称从采集器中移除指定设备
 * @param deviceName const String& 要移除的设备名称
 * 
 * 移除流程：
 * 1. 查找指定名称的设备
 * 2. 如果找到，先停止设备再移除
 * 3. 从设备列表中删除
 */
void DataCollector::removeDevice(const String& deviceName) {
    try {
        // 在设备列表中查找指定名称的设备
        auto it = std::find_if(devices_.begin(), devices_.end(),
            [&deviceName](const std::shared_ptr<Device>& device) {
                return device->getName() == deviceName;
            });
        
        if (it != devices_.end()) {
            // 找到设备，先停止设备运行
            (*it)->stop();
            
            // 从列表中移除设备
            devices_.erase(it);
            std::cout << "设备 " << deviceName << " 已从采集器移除" << std::endl;
        } else {
            // 设备不存在
            std::cout << "设备 " << deviceName << " 不存在" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "移除设备失败: " << e.what() << std::endl;
    }
}

/**
 * @brief 启动数据采集线程
 * @details 创建并启动专门的数据采集线程
 * 
 * 线程管理：
 * - 如果已有线程在运行，先等待其结束
 * - 创建新的采集线程执行 collectLoop 函数
 */
void DataCollector::startThread() {
    // 确保之前的线程已经完全结束
    if (collectorThread_.joinable()) {
        collectorThread_.join();
    }
    
    // 创建新的采集线程，执行 collectLoop 成员函数
    collectorThread_ = std::thread(&DataCollector::collectLoop, this);
}

/**
 * @brief 停止数据采集线程
 * @details 等待采集线程正常结束
 * 
 * 线程停止：
 * - 等待线程自然结束（join）
 * - 确保资源正确释放
 */
void DataCollector::stopThread() {
    if (collectorThread_.joinable()) {
        collectorThread_.join();
    }
}

/**
 * @brief 数据采集循环（线程主函数）
 * @details 采集线程的执行入口，负责持续的数据采集工作
 * 
 * 采集循环流程：
 * 1. 记录线程启动日志
 * 2. 循环执行数据采集更新
 * 3. 按配置的间隔等待下次采集
 * 4. 异常处理和恢复
 * 5. 记录线程结束日志
 */
void DataCollector::collectLoop() {
    std::cout << "数据采集线程已启动" << std::endl;
    
    // 主采集循环
    while (running_) {
        try {
            // 执行数据采集更新
            updateLoop();
            
            // 等待下次采集（使用配置的采集间隔）
            std::this_thread::sleep_for(collectionInterval_);
            
        } catch (const std::exception& e) {
            // 异常处理：记录错误并等待1秒后继续
            std::cerr << "数据采集循环异常: " << e.what() << std::endl;
            std::this_thread::sleep_for(Duration(1000));
        }
    }
    
    std::cout << "数据采集线程已停止" << std::endl;
}

/**
 * @brief 数据采集更新循环
 * @details 执行实际的数据采集工作，检查设备状态
 * 
 * 更新循环内容：
 * 1. 遍历所有设备，检查运行状态
 * 2. 监控设备状态变化
 * 3. 定期输出采集状态统计
 * 
 * 注意：
 * - 实际的数据采集逻辑在设备类中实现
 * - 这里主要负责设备管理和状态监控
 */
void DataCollector::updateLoop() {
    // 遍历所有设备，检查状态
    for (auto& device : devices_) {
        try {
            if (device && device->isRunning()) {
                // 设备正在运行，可以在这里添加数据采集逻辑
                // 例如：从设备读取数据、处理数据等
                
                // 这里暂时只是状态检查，实际的数据采集会在设备类中实现
                if (device->getStatus() == DeviceStatus::ERROR) {
                    std::cout << "设备 " << device->getName() << " 状态异常" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "检查设备 " << device->getName() << " 状态时出错: " << e.what() << std::endl;
        }
    }
    
    // 定期输出采集状态统计（每10次循环输出一次）
    static int counter = 0;
    if (++counter % 10 == 0) {
        std::cout << "数据采集状态: " << devices_.size() << " 个设备在线" << std::endl;
    }
}

} // namespace plc
