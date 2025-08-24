#pragma once

#include "common.h"
#include <random>
#include <memory>

namespace plc {

// 数据模拟器基类
class DataSimulator {
public:
    explicit DataSimulator(const String& name);
    virtual ~DataSimulator() = default;
    
    // 生成模拟数据
    virtual DataPoint generateData() = 0;
    
    // 设置参数
    void setMinValue(double min) { minValue_ = min; }
    void setMaxValue(double max) { maxValue_ = max; }
    void setUpdateInterval(Duration interval) { updateInterval_ = interval; }
    
    // 获取参数
    double getMinValue() const { return minValue_; }
    double getMaxValue() const { return maxValue_; }
    Duration getUpdateInterval() const { return updateInterval_; }
    String getName() const { return name_; }
    
protected:
    String name_;
    double minValue_;
    double maxValue_;
    Duration updateInterval_;
    std::random_device randomDevice_;
    std::mt19937 randomGenerator_;
    std::uniform_real_distribution<double> randomDistribution_;
    
    // 生成随机值
    double generateRandomValue();
    double generateRandomValue(double min, double max);
};

// 温度数据模拟器
class TemperatureSimulator : public DataSimulator {
public:
    explicit TemperatureSimulator(const String& name = "Temperature");
    
    DataPoint generateData() override;
    
private:
    double baseTemperature_;
    double variation_;
    double trend_;
    int trendCounter_;
};

// 压力数据模拟器
class PressureSimulator : public DataSimulator {
public:
    explicit PressureSimulator(const String& name = "Pressure");
    
    DataPoint generateData() override;
    
private:
    double basePressure_;
    double noise_;
    int cycleCounter_;
};

// 流量数据模拟器
class FlowSimulator : public DataSimulator {
public:
    explicit FlowSimulator(const String& name = "Flow");
    
    DataPoint generateData() override;
    
private:
    double baseFlow_;
    double fluctuation_;
    int seasonalCounter_;
};

// 状态数据模拟器
class StatusSimulator : public DataSimulator {
public:
    explicit StatusSimulator(const String& name = "Status");
    
    DataPoint generateData() override;
    
private:
    int currentStatus_;
    std::vector<String> statusValues_;
    int statusCounter_;
};

// 复合数据模拟器
class CompositeSimulator {
public:
    explicit CompositeSimulator(const String& name);
    
    // 添加模拟器
    void addSimulator(std::shared_ptr<DataSimulator> simulator);
    
    // 生成所有数据
    std::vector<DataPoint> generateAllData();
    
    // 生成指定类型的数据
    DataPoint generateData(DataType type);
    
    // 获取模拟器列表
    const std::vector<std::shared_ptr<DataSimulator>>& getSimulators() const { return simulators_; }
    
private:
    String name_;
    std::vector<std::shared_ptr<DataSimulator>> simulators_;
    std::map<DataType, std::shared_ptr<DataSimulator>> typeMap_;
};

} // namespace plc
