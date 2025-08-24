/**
 * @file modbus_device.cpp
 * @brief Modbus设备实现文件
 * @details 实现了Modbus协议的工业设备模拟，包括寄存器管理、数据生成和通信处理
 * @author 工业设备数据模拟系统开发团队
 * @date 2025
 * @version 1.0.0
 */

#include "../../src/modbus/modbus_device.h"
#include "../../include/common.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace plc {

/**
 * @brief Modbus设备构造函数
 * @details 初始化Modbus设备的基本属性和寄存器
 * @param config DeviceConfig 设备配置信息
 * 
 * 初始化内容：
 * - 继承基类Device的配置
 * - 初始化随机数生成器
 * - 设置基础数据值（温度、压力、流量、状态）
 * - 记录最后更新时间
 * - 设置更新间隔
 * - 初始化Modbus寄存器
 */
ModbusDevice::ModbusDevice(const DeviceConfig& config)
    : Device(config)                           // 调用基类构造函数
    , randomGenerator_(randomDevice_())        // 初始化随机数生成器
    , randomDistribution_(0.0, 1.0)           // 设置随机数分布范围（0-1）
    , temperatureBase_(25.0)                   // 基础温度值25°C
    , pressureBase_(1.0)                       // 基础压力值1.0 MPa
    , flowBase_(100.0)                         // 基础流量值100 L/min
    , statusValue_(0)                          // 初始状态值0（正常）
    , lastUpdate_(Utils::getCurrentTime())     // 记录当前时间作为最后更新时间
    , updateInterval_(config.updateInterval) { // 设置更新间隔
    
    std::cout << "Modbus设备 " << config.name << " 已创建" << std::endl;
    initializeRegisters();  // 初始化Modbus寄存器
}

/**
 * @brief Modbus设备析构函数
 * @details 确保设备在销毁前正确清理资源
 */
ModbusDevice::~ModbusDevice() {
    cleanup();  // 调用清理函数
}

/**
 * @brief 初始化Modbus设备
 * @details 执行Modbus设备的初始化操作
 * @return ErrorCode 初始化结果错误码
 * 
 * 初始化流程：
 * 1. 设置寄存器初始值
 * 2. 启动Modbus服务器（模拟）
 * 3. 返回初始化结果
 */
ErrorCode ModbusDevice::initialize() {
    try {
        std::cout << "初始化Modbus设备 " << config_.name << "..." << std::endl;
        
        // 第一步：初始化寄存器值
        setRegisterValue(0, temperatureBase_);  // 温度寄存器（地址0）
        setRegisterValue(2, pressureBase_);     // 压力寄存器（地址2）
        setRegisterValue(4, flowBase_);         // 流量寄存器（地址4）
        setRegisterValue(6, statusValue_);      // 状态寄存器（地址6）
        
        // 第二步：启动Modbus服务器（模拟）
        if (!startModbusServer()) {
            std::cerr << "Modbus服务器启动失败" << std::endl;
            return ErrorCode::CONNECTION_FAILED;
        }
        
        std::cout << "Modbus设备 " << config_.name << " 初始化成功" << std::endl;
        return ErrorCode::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "Modbus设备初始化异常: " << e.what() << std::endl;
        return ErrorCode::UNKNOWN_ERROR;
    }
}

/**
 * @brief 清理Modbus设备资源
 * @details 停止Modbus服务器并清理相关资源
 * @return ErrorCode 清理结果错误码
 */
ErrorCode ModbusDevice::cleanup() {
    try {
        stopModbusServer();  // 停止Modbus服务器
        std::cout << "Modbus设备 " << config_.name << " 清理完成" << std::endl;
        return ErrorCode::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "Modbus设备清理异常: " << e.what() << std::endl;
        return ErrorCode::UNKNOWN_ERROR;
    }
}

/**
 * @brief Modbus设备运行循环
 * @details 设备的主要运行循环，负责数据生成、请求处理和状态更新
 * 
 * 运行循环流程：
 * 1. 生成模拟数据
 * 2. 处理Modbus请求（模拟）
 * 3. 发送数据到回调函数
 * 4. 等待下次更新间隔
 * 5. 异常处理和状态更新
 */
void ModbusDevice::runLoop() {
    std::cout << "Modbus设备 " << config_.name << " 运行循环启动" << std::endl;
    
    while (running_) {
        try {
            // 第一步：生成模拟数据
            generateSimulatedData();
            
            // 第二步：处理Modbus请求（模拟）
            handleModbusRequest();
            
            // 第三步：发送数据到回调函数
            sendDataToCallback();
            
            // 第四步：等待下次更新
            std::this_thread::sleep_for(updateInterval_);
            
        } catch (const std::exception& e) {
            // 异常处理：记录错误并更新设备状态
            std::cerr << "Modbus设备运行异常: " << e.what() << std::endl;
            updateStatus(DeviceStatus::ERROR);
            std::this_thread::sleep_for(Duration(1000));
        }
    }
    
    std::cout << "Modbus设备 " << config_.name << " 运行循环已停止" << std::endl;
}

/**
 * @brief 初始化Modbus寄存器
 * @details 创建和配置标准的Modbus寄存器
 * 
 * 寄存器配置：
 * - 地址0：温度寄存器（15.0-35.0°C）
 * - 地址2：压力寄存器（0.8-1.2 MPa）
 * - 地址4：流量寄存器（80.0-120.0 L/min）
 * - 地址6：状态寄存器（0.0-4.0 状态码）
 */
void ModbusDevice::initializeRegisters() {
    // 添加标准的Modbus寄存器
    addRegister(ModbusRegister(0, DataType::TEMPERATURE, "温度", "°C", 15.0, 35.0));
    addRegister(ModbusRegister(2, DataType::PRESSURE, "压力", "MPa", 0.8, 1.2));
    addRegister(ModbusRegister(4, DataType::FLOW, "流量", "L/min", 80.0, 120.0));
    addRegister(ModbusRegister(6, DataType::STATUS, "状态", "状态码", 0.0, 4.0));
}

void ModbusDevice::setRegisterValue(int address, double value) {
    auto it = registers_.find(address);
    if (it != registers_.end()) {
        it->second.currentValue = value;
    } else {
        std::cerr << "寄存器地址 " << address << " 不存在" << std::endl;
    }
}

double ModbusDevice::getRegisterValue(int address) const {
    auto it = registers_.find(address);
    if (it != registers_.end()) {
        return it->second.currentValue;
    }
    return 0.0;
}

void ModbusDevice::addRegister(const ModbusRegister& reg) {
    registers_[reg.address] = reg;
    std::cout << "添加Modbus寄存器: 地址=" << reg.address 
              << ", 类型=" << reg.name 
              << ", 范围=[" << reg.minValue << ", " << reg.maxValue << "]" << std::endl;
}

void ModbusDevice::generateSimulatedData() {
    auto now = Utils::getCurrentTime();
    
    // 模拟温度数据
    simulateTemperature();
    
    // 模拟压力数据
    simulatePressure();
    
    // 模拟流量数据
    simulateFlow();
    
    // 模拟状态数据
    simulateStatus();
    
    lastUpdate_ = now;
}

void ModbusDevice::simulateTemperature() {
    // 基础温度变化
    double variation = 2.0 * (randomDistribution_(randomGenerator_) - 0.5);
    double newTemp = temperatureBase_ + variation;
    
    // 添加昼夜变化
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    int hour = tm.tm_hour;
    double dayNightVariation = 3.0 * sin((hour - 6) * M_PI / 12.0);
    newTemp += dayNightVariation;
    
    // 限制在合理范围内
    auto& tempReg = registers_[0];
    newTemp = std::max(tempReg.minValue, std::min(tempReg.maxValue, newTemp));
    
    setRegisterValue(0, newTemp);
}

void ModbusDevice::simulatePressure() {
    // 基础压力 + 随机噪声
    double noise = 0.05 * (randomDistribution_(randomGenerator_) - 0.5);
    double newPressure = pressureBase_ + noise;
    
    // 添加周期性变化
    static int cycleCounter = 0;
    cycleCounter++;
    double cycleVariation = 0.02 * sin(cycleCounter * 0.1);
    newPressure += cycleVariation;
    
    // 限制在合理范围内
    auto& pressureReg = registers_[2];
    newPressure = std::max(pressureReg.minValue, std::min(pressureReg.maxValue, newPressure));
    
    setRegisterValue(2, newPressure);
}

void ModbusDevice::simulateFlow() {
    // 基础流量 + 随机波动
    double fluctuation = 10.0 * (randomDistribution_(randomGenerator_) - 0.5);
    double newFlow = flowBase_ + fluctuation;
    
    // 添加季节性变化
    static int seasonCounter = 0;
    seasonCounter++;
    double seasonalVariation = 5.0 * sin(seasonCounter * 0.01);
    newFlow += seasonalVariation;
    
    // 限制在合理范围内
    auto& flowReg = registers_[4];
    newFlow = std::max(flowReg.minValue, std::min(flowReg.maxValue, newFlow));
    
    setRegisterValue(4, newFlow);
}

void ModbusDevice::simulateStatus() {
    // 状态偶尔变化
    static int statusCounter = 0;
    statusCounter++;
    
    if (statusCounter % 100 == 0) {
        // 90% 概率保持正常，10% 概率变为其他状态
        if (randomDistribution_(randomGenerator_) < 0.9) {
            statusValue_ = 0; // 正常
        } else {
            statusValue_ = static_cast<int>(randomDistribution_(randomGenerator_) * 4); // 0-3
        }
        setRegisterValue(6, statusValue_);
    }
}

bool ModbusDevice::startModbusServer() {
    // 模拟Modbus服务器启动
    // 在真实实现中，这里会启动TCP服务器监听502端口
    std::cout << "Modbus服务器已启动，监听地址: " << config_.address << ":" << config_.port << std::endl;
    return true;
}

void ModbusDevice::stopModbusServer() {
    // 模拟Modbus服务器停止
    std::cout << "Modbus服务器已停止" << std::endl;
}

void ModbusDevice::handleModbusRequest() {
    // 模拟处理Modbus请求
    // 在真实实现中，这里会解析Modbus协议包，读取/写入寄存器
    
    // 这里只是简单地更新寄存器值，模拟数据更新
    static int requestCounter = 0;
    requestCounter++;
    
    if (requestCounter % 10 == 0) {
        std::cout << "处理Modbus请求 #" << requestCounter << std::endl;
    }
}

void ModbusDevice::sendDataToCallback() {
    // 发送各个寄存器的数据到回调函数
    for (const auto& pair : registers_) {
        const ModbusRegister& reg = pair.second;
        
        DataPoint data(reg.type, reg.currentValue, reg.unit, config_.name);
        sendData(data);
    }
}

// Modbus协议相关的工具函数
namespace ModbusProtocol {

// Modbus功能码
enum class FunctionCode : uint8_t {
    READ_COILS = 0x01,
    READ_DISCRETE_INPUTS = 0x02,
    READ_HOLDING_REGISTERS = 0x03,
    READ_INPUT_REGISTERS = 0x04,
    WRITE_SINGLE_COIL = 0x05,
    WRITE_SINGLE_REGISTER = 0x06,
    WRITE_MULTIPLE_COILS = 0x0F,
    WRITE_MULTIPLE_REGISTERS = 0x10
};

// Modbus PDU结构
struct ModbusPDU {
    uint8_t functionCode;
    uint16_t startAddress;
    uint16_t quantity;
    uint8_t data[256];
    uint8_t dataLength;
};

// 计算CRC16校验码
uint16_t calculateCRC16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

// 解析Modbus请求
bool parseModbusRequest(const uint8_t* buffer, size_t length, ModbusPDU& pdu) {
    if (length < 6) {
        return false; // 最小长度检查
    }
    
    // 跳过MBAP头（如果是TCP）或从机地址（如果是RTU）
    size_t offset = 0;
    if (length > 8) {
        offset = 6; // TCP MBAP头长度
    } else {
        offset = 1; // RTU从机地址
    }
    
    pdu.functionCode = buffer[offset];
    pdu.startAddress = (buffer[offset + 1] << 8) | buffer[offset + 2];
    pdu.quantity = (buffer[offset + 3] << 8) | buffer[offset + 4];
    
    return true;
}

// 构建Modbus响应
size_t buildModbusResponse(const ModbusPDU& request, const std::vector<uint16_t>& data, 
                          uint8_t* buffer, size_t bufferSize) {
    if (bufferSize < 5 + data.size() * 2) {
        return 0; // 缓冲区太小
    }
    
    size_t offset = 0;
    
    // 功能码
    buffer[offset++] = request.functionCode;
    
    // 数据长度（字节数）
    buffer[offset++] = static_cast<uint8_t>(data.size() * 2);
    
    // 数据
    for (uint16_t value : data) {
        buffer[offset++] = static_cast<uint8_t>(value >> 8);
        buffer[offset++] = static_cast<uint8_t>(value & 0xFF);
    }
    
    return offset;
}

} // namespace ModbusProtocol

} // namespace plc
