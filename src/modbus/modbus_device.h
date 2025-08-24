#ifndef MODBUS_DEVICE_H
#define MODBUS_DEVICE_H

#include "device.h"
#include <map>
#include <random>

namespace plc {

// Modbus寄存器配置
struct ModbusRegister {
    int address;
    DataType type;
    String name;
    String unit;
    double minValue;
    double maxValue;
    double currentValue;
    
    // 默认构造函数（为std::map兼容性）
    ModbusRegister() : address(0), type(DataType::CUSTOM), name(""), unit(""), minValue(0.0), maxValue(100.0), currentValue(0.0) {}
    
    // 参数化构造函数
    ModbusRegister(int addr, DataType t, const String& n, const String& u, double min, double max)
        : address(addr), type(t), name(n), unit(u), minValue(min), maxValue(max), currentValue(0.0) {}
};

// Modbus设备类
class ModbusDevice : public Device {
public:
    explicit ModbusDevice(const DeviceConfig& config);
    virtual ~ModbusDevice();
    
    // 重写基类方法
    ErrorCode initialize() override;
    ErrorCode cleanup() override;
    void runLoop() override;
    
    // Modbus特定方法
    void setRegisterValue(int address, double value);
    double getRegisterValue(int address) const;
    void addRegister(const ModbusRegister& reg);
    
    // 模拟数据生成
    void generateSimulatedData();
    
private:
    // 初始化寄存器
    void initializeRegisters();
    
    // Modbus服务器相关
    bool startModbusServer();
    void stopModbusServer();
    void handleModbusRequest();
    
    // 数据发送
    void sendDataToCallback();
    
    // 数据模拟
    void simulateTemperature();
    void simulatePressure();
    void simulateFlow();
    void simulateStatus();
    
    // 成员变量
    std::map<int, ModbusRegister> registers_;
    std::random_device randomDevice_;
    std::mt19937 randomGenerator_;
    std::uniform_real_distribution<double> randomDistribution_;
    
    // 模拟参数
    double temperatureBase_;
    double pressureBase_;
    double flowBase_;
    int statusValue_;
    
    // 时间控制
    TimePoint lastUpdate_;
    Duration updateInterval_;
};

} // namespace plc

#endif // MODBUS_DEVICE_H
