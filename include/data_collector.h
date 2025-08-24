#pragma once

#include "common.h"
#include "device.h"
#include <vector>
#include <memory>
#include <atomic>

namespace plc {

// 数据采集器类
class DataCollector {
public:
    DataCollector();
    ~DataCollector();
    
    // 基本操作
    bool initialize();
    bool start();
    bool stop();
    void cleanup();
    
    // 设备管理
    void addDevice(std::shared_ptr<Device> device);
    void removeDevice(const String& deviceName);
    
    // 状态查询
    bool isRunning() const { return running_; }
    size_t getDeviceCount() const { return devices_.size(); }
    
private:
    // 数据采集循环
    void collectLoop();
    
    // 线程控制
    void startThread();
    void stopThread();
    
    // 更新循环
    void updateLoop();
    
    // 成员变量
    std::vector<std::shared_ptr<Device>> devices_;
    std::atomic<bool> running_;
    std::thread collectorThread_;
    
    // 配置
    Duration collectionInterval_;
};

} // namespace plc

