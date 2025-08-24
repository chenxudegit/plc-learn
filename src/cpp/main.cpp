/**
 * @file main.cpp
 * @brief 工业设备数据模拟系统主程序
 * @details 系统的主入口点，负责初始化、运行和清理整个系统
 * @author 工业设备数据模拟系统开发团队
 * @date 2025
 * @version 1.0.0
 */

#include <iostream>
#include <memory>
#include <signal.h>
#include <chrono>
#include <thread>
#include "../../include/common.h"
#include "../../include/data_collector.h"
#include "../../include/data_simulator.h"
#include "../../include/database_manager.h"  
#include "../../include/device.h"
#include "../../src/modbus/modbus_device.h"
#include "../../src/socket/socket_device.h"

using namespace plc;

// ==================== 全局变量定义 ====================

/**
 * @brief 系统运行标志
 * @details 原子布尔变量，用于控制系统的运行状态
 * 可以通过信号处理函数设置为false来优雅地关闭系统
 */
std::atomic<bool> running(true);

/**
 * @brief 数据采集器实例
 * @details 负责管理和协调多个设备的数据采集
 */
std::unique_ptr<DataCollector> dataCollector;

/**
 * @brief 复合数据模拟器实例
 * @details 管理多种类型的数据模拟器，生成模拟数据
 */
std::unique_ptr<CompositeSimulator> compositeSimulator;

/**
 * @brief 数据库管理器实例
 * @details 负责数据的存储、查询和管理
 */
std::unique_ptr<DatabaseManager> dbManager; 

/**
 * @brief Modbus设备实例
 * @details 模拟Modbus协议的工业设备
 */
std::shared_ptr<ModbusDevice> modbusDevice;

/**
 * @brief Socket设备实例
 * @details 模拟Socket通信的工业设备
 */
std::shared_ptr<SocketDevice> socketDevice;

// ==================== 信号处理函数 ====================

/**
 * @brief 信号处理函数
 * @details 处理系统信号，实现优雅关闭
 * @param signum int 接收到的信号编号
 * 
 * 支持的信号：
 * - SIGINT: Ctrl+C 中断信号
 * - SIGTERM: 终止信号
 */
void signalHandler(int signum) {
    std::cout << "接收到信号 " << signum << "，正在关闭系统..." << std::endl;
    running = false;  // 设置运行标志为false，触发系统关闭流程
}

// ==================== 回调函数 ====================

/**
 * @brief 数据接收回调函数
 * @details 当设备产生新数据时被调用
 * @param data const DataPoint& 接收到的数据点
 * 
 * 处理流程：
 * 1. 输出数据信息到控制台
 * 2. 将数据存储到数据库
 */
void onDataReceived(const DataPoint& data) {
    // 输出数据信息到控制台
    std::cout << "收到数据: " << data.source << " - " 
              << Utils::dataTypeToString(data.type) << ": " 
              << data.value << " " << data.unit << std::endl;
    
    // 将数据存储到数据库
    if (dbManager) {
        dbManager->insertDataPoint(data);
    }
}

/**
 * @brief 设备状态变化回调函数
 * @details 当设备状态发生变化时被调用
 * @param status DeviceStatus 新的设备状态
 */
void onStatusChanged(DeviceStatus status) {
    std::cout << "设备状态变化: " << static_cast<int>(status) << std::endl;
}

// ==================== 初始化函数 ====================

/**
 * @brief 初始化数据模拟器
 * @details 创建和配置各种类型的数据模拟器
 * 
 * 初始化的模拟器类型：
 * - 温度模拟器：模拟温度传感器数据
 * - 压力模拟器：模拟压力传感器数据
 * - 流量模拟器：模拟流量计数据
 * - 状态模拟器：模拟设备状态数据
 */
void initializeDataSimulators() {
    std::cout << "初始化数据模拟器..." << std::endl;
    
    // 创建复合模拟器，用于管理多个模拟器
    compositeSimulator = std::make_unique<CompositeSimulator>("工业设备模拟器");
    
    // 创建各种类型的模拟器
    auto tempSim = std::make_shared<TemperatureSimulator>("温度传感器");
    auto pressureSim = std::make_shared<PressureSimulator>("压力传感器");
    auto flowSim = std::make_shared<FlowSimulator>("流量计");
    auto statusSim = std::make_shared<StatusSimulator>("设备状态");
    
    // 将模拟器添加到复合模拟器中
    compositeSimulator->addSimulator(tempSim);
    compositeSimulator->addSimulator(pressureSim);
    compositeSimulator->addSimulator(flowSim);
    compositeSimulator->addSimulator(statusSim);
    
    std::cout << "数据模拟器初始化完成，共 " << compositeSimulator->getSimulators().size() << " 个模拟器" << std::endl;
}

/**
 * @brief 数据生成循环
 * @details 在独立线程中持续生成模拟数据
 * 
 * 循环流程：
 * 1. 生成所有类型的数据
 * 2. 将数据存储到数据库
 * 3. 输出数据统计信息
 * 4. 等待下次生成间隔
 */
void dataGenerationLoop() {
    std::cout << "启动数据生成循环..." << std::endl;
    
    int counter = 0;
    while (running) {
        try {
            // 第一步：生成所有类型的数据
            auto allData = compositeSimulator->generateAllData();
            
            // 第二步：将数据存储到数据库
            for (const auto& data : allData) {
                if (dbManager) {
                    dbManager->insertDataPoint(data);
                }
            }
            
            // 第三步：输出数据统计信息（每10次循环输出一次）
            if (++counter % 10 == 0) {
                std::cout << "已生成 " << counter * allData.size() << " 个数据点" << std::endl;
            }
            
            // 第四步：模拟数据处理延迟（1秒）
            std::this_thread::sleep_for(Duration(1000));
            
        } catch (const std::exception& e) {
            // 异常处理：记录错误并等待1秒后继续
            std::cerr << "数据生成异常: " << e.what() << std::endl;
            std::this_thread::sleep_for(Duration(1000));
        }
    }
    
    std::cout << "数据生成循环已停止" << std::endl;
}

/**
 * @brief 初始化真实设备
 * @details 创建和配置Modbus和Socket设备
 * 
 * 初始化的设备：
 * - Modbus设备：模拟Modbus协议通信
 * - Socket设备：模拟Socket通信
 */
void initializeRealDevices() {
    std::cout << "初始化真实设备..." << std::endl;
    
    try {
        // 第一步：创建Modbus设备配置
        DeviceConfig modbusConfig;
        modbusConfig.name = "Modbus设备";
        modbusConfig.address = "127.0.0.1";      // 本地地址
        modbusConfig.port = 502;                 // Modbus标准端口
        modbusConfig.updateInterval = Duration(2000);  // 2秒更新间隔
        modbusConfig.enabled = true;             // 启用设备
        
        // 第二步：创建Modbus设备实例
        modbusDevice = std::make_shared<ModbusDevice>(modbusConfig);
        
        // 第三步：设置数据回调函数
        modbusDevice->setDataCallback(onDataReceived);
        modbusDevice->setStatusCallback(onStatusChanged);
        
        // 第四步：记录设备状态到数据库
        if (dbManager) {
            dbManager->insertDeviceStatus("Modbus设备", DeviceStatus::ONLINE);
        }
        
        std::cout << "Modbus设备创建成功" << std::endl;
        
        // 第五步：创建Socket设备配置
        DeviceConfig socketConfig;
        socketConfig.name = "Socket设备";
        socketConfig.address = "127.0.0.1";      // 本地地址
        socketConfig.port = 8080;                // Socket端口
        socketConfig.updateInterval = Duration(1000);  // 1秒更新间隔
        socketConfig.enabled = true;             // 启用设备
        
        // 第六步：创建Socket设备实例
        socketDevice = std::make_shared<SocketDevice>(socketConfig);
        
        // 第七步：设置数据回调函数
        socketDevice->setDataCallback(onDataReceived);
        socketDevice->setStatusCallback(onStatusChanged);
        
        // 第八步：记录设备状态到数据库
        if (dbManager) {
            dbManager->insertDeviceStatus("Socket设备", DeviceStatus::ONLINE);
        }
        
        std::cout << "Socket设备创建成功" << std::endl;
        
        std::cout << "真实设备初始化完成" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "初始化真实设备异常: " << e.what() << std::endl;
    }
}

// ==================== 主函数 ====================

/**
 * @brief 主函数
 * @details 系统的主入口点，负责整个系统的生命周期管理
 * @return int 程序退出码（0表示成功，-1表示失败）
 * 
 * 主函数流程：
 * 1. 系统启动和版本信息显示
 * 2. 设置信号处理
 * 3. 初始化各个组件
 * 4. 启动系统运行
 * 5. 主循环监控
 * 6. 优雅关闭和资源清理
 */
int main() {
    // 第一步：显示系统启动信息
    std::cout << "=== 工业设备数据模拟系统 ===" << std::endl;
    std::cout << "版本: 1.0.0" << std::endl;
    std::cout << "启动时间: " << Utils::getCurrentTimeString() << std::endl;
    
    // 第二步：设置信号处理，支持优雅关闭
    signal(SIGINT, signalHandler);   // Ctrl+C 信号
    signal(SIGTERM, signalHandler);  // 终止信号
    
    try {
        // 第三步：初始化数据库管理器
        dbManager = std::make_unique<DatabaseManager>();
        if (!dbManager->initialize()) {
            std::cerr << "数据库管理器初始化失败" << std::endl;
            return -1;
        }
        std::cout << "数据库管理器初始化成功" << std::endl;
        
        // 第四步：初始化数据模拟器
        initializeDataSimulators();
        
        // 第五步：初始化真实设备
        initializeRealDevices();
        
        // 第六步：初始化数据采集器
        dataCollector = std::make_unique<DataCollector>();
        if (!dataCollector->initialize()) {
            std::cerr << "数据采集器初始化失败" << std::endl;
            return -1;
        }
        std::cout << "数据采集器初始化成功" << std::endl;
        
        // 第七步：将设备添加到采集器中
        if (modbusDevice) {
            dataCollector->addDevice(modbusDevice);
        }
        if (socketDevice) {
            dataCollector->addDevice(socketDevice);
        }
        
        // 第八步：启动数据采集器
        if (!dataCollector->start()) {
            std::cerr << "数据采集器启动失败" << std::endl;
            return -1;
        }
        std::cout << "数据采集器启动成功" << std::endl;
        
        // 第九步：启动数据生成线程（独立线程）
        std::thread dataGenThread(dataGenerationLoop);
        
        // 第十步：主循环 - 系统运行监控
        std::cout << "系统运行中，按 Ctrl+C 退出..." << std::endl;
        while (running) {
            // 主循环：每秒检查一次运行状态
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // 定期显示系统状态（每10秒一次）
            static int counter = 0;
            if (++counter % 10 == 0) {
                std::cout << "系统运行中... " << Utils::getCurrentTimeString() << std::endl;
                
                // 生成并显示一些示例数据
                if (compositeSimulator) {
                    auto sampleData = compositeSimulator->generateAllData();
                    std::cout << "当前数据样本:" << std::endl;
                    for (const auto& data : sampleData) {
                        std::cout << "  " << data.source << ": " 
                                  << Utils::dataTypeToString(data.type) << " = " 
                                  << data.value << " " << data.unit << std::endl;
                    }
                }
            }
        }
        
        // 第十一步：等待数据生成线程结束
        if (dataGenThread.joinable()) {
            dataGenThread.join();
        }
        
    } catch (const std::exception& e) {
        // 异常处理：记录错误并返回失败码
        std::cerr << "系统异常: " << e.what() << std::endl;
        return -1;
    }
    
    // 第十二步：系统关闭和资源清理
    std::cout << "正在清理资源..." << std::endl;
    
    // 停止数据采集器
    if (dataCollector) {
        dataCollector->stop();
    }
    
    // 清理数据库管理器
    if (dbManager) {
        dbManager->cleanup();
    }
    
    // 停止所有设备
    if (modbusDevice) {
        modbusDevice->stop();
    }
    
    if (socketDevice) {
        socketDevice->stop();
    }
    
    std::cout << "系统已安全关闭" << std::endl;
    return 0; 
}
