#pragma once

#include "common.h"
#include <functional>
#include <thread>
#include <atomic>

namespace plc {

// 数据回调函数类型
using DataCallback = std::function<void(const DataPoint&)>;
using StatusCallback = std::function<void(DeviceStatus)>;

// 设备基类
class Device {
public:
    explicit Device(const DeviceConfig& config);
    virtual ~Device();
    
    // 禁用拷贝构造和赋值
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    
    // 基本操作
    virtual ErrorCode start();
    virtual ErrorCode stop();
    virtual ErrorCode reset();
    
    // 状态查询
    DeviceStatus getStatus() const { return status_; }
    String getName() const { return config_.name; }
    bool isRunning() const { return running_; }
    
    // 回调设置
    void setDataCallback(DataCallback callback) { dataCallback_ = callback; }
    void setStatusCallback(StatusCallback callback) { statusCallback_ = callback; }
    
    // 配置管理
    const DeviceConfig& getConfig() const { return config_; }
    void updateConfig(const DeviceConfig& config);
    
protected:
    // 子类需要实现的虚函数
    virtual ErrorCode initialize() = 0;
    virtual ErrorCode cleanup() = 0;
    virtual void runLoop() = 0;
    
    // 数据发送
    void sendData(const DataPoint& data);
    void updateStatus(DeviceStatus status);
    
    // 线程控制
    void startThread();
    void stopThread();
    
    // 成员变量
    DeviceConfig config_;
    DeviceStatus status_;
    std::atomic<bool> running_;
    std::thread workerThread_;
    
    // 回调函数
    DataCallback dataCallback_;
    StatusCallback statusCallback_;
    
    // 内部方法
    void workerFunction();
    void updateLoop();
};

} // namespace plc
