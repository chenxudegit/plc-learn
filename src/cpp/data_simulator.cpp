/**
 * @file data_simulator.cpp
 * @brief 数据模拟器实现文件
 * @details 实现了多种类型的数据模拟器，用于生成工业设备的模拟数据
 * @author 工业设备数据模拟系统开发团队
 * @date 2025
 * @version 1.0.0
 */

#include "data_simulator.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace plc {

// ==================== DataSimulator 基类 ====================

/**
 * @brief 数据模拟器基类构造函数
 * @details 初始化模拟器的基本属性
 * @param name const String& 模拟器名称
 * 
 * 初始化内容：
 * - 设置模拟器名称
 * - 设置默认数值范围（0.0 - 100.0）
 * - 设置默认更新间隔（1秒）
 * - 初始化随机数生成器
 * - 设置随机数分布范围（0.0 - 1.0）
 */
DataSimulator::DataSimulator(const String& name)
    : name_(name)                         // 模拟器名称
    , minValue_(0.0)                      // 最小值
    , maxValue_(100.0)                    // 最大值
    , updateInterval_(Duration(1000))     // 更新间隔1秒
    , randomGenerator_(randomDevice_())   // 随机数生成器
    , randomDistribution_(0.0, 1.0) {    // 随机数分布（0-1）
}

/**
 * @brief 生成0-1之间的随机值
 * @details 使用均匀分布生成0.0到1.0之间的随机浮点数
 * @return double 随机浮点数值
 */
double DataSimulator::generateRandomValue() {
    return randomDistribution_(randomGenerator_);
}

/**
 * @brief 生成指定范围内的随机值
 * @details 在指定的最小值和最大值之间生成随机浮点数
 * @param min double 最小值
 * @param max double 最大值
 * @return double 指定范围内的随机浮点数值
 */
double DataSimulator::generateRandomValue(double min, double max) {
    return min + (max - min) * generateRandomValue();
}

// ==================== TemperatureSimulator 温度模拟器 ====================

/**
 * @brief 温度模拟器构造函数
 * @details 初始化温度模拟器的特定参数
 * @param name const String& 模拟器名称
 * 
 * 温度模拟器特点：
 * - 基础温度：25°C
 * - 变化幅度：±5°C
 * - 更新间隔：2秒（温度变化较慢）
 * - 数值范围：15°C - 35°C
 */
TemperatureSimulator::TemperatureSimulator(const String& name)
    : DataSimulator(name)
    , baseTemperature_(25.0)              // 基础温度25°C
    , variation_(5.0)                     // 随机变化幅度±5°C
    , trend_(0.0)                         // 趋势变化系数
    , trendCounter_(0) {                  // 趋势计数器
    
    setMinValue(15.0);                    // 设置最小温度15°C
    setMaxValue(35.0);                    // 设置最大温度35°C
    setUpdateInterval(Duration(2000));    // 温度变化较慢，2秒更新一次
}

/**
 * @brief 生成温度数据点
 * @details 生成包含多种变化模式的温度数据
 * @return DataPoint 温度数据点
 * 
 * 温度变化模式：
 * 1. 基础温度：25°C
 * 2. 随机变化：±5°C的随机波动
 * 3. 趋势变化：缓慢的上升或下降趋势
 * 4. 昼夜变化：模拟真实环境的昼夜温差
 */
DataPoint TemperatureSimulator::generateData() {
    // 第一步：设置基础温度
    double temperature = baseTemperature_;
    
    // 第二步：添加随机变化（±5°C）
    temperature += generateRandomValue(-variation_, variation_);
    
    // 第三步：添加趋势变化（缓慢上升或下降）
    trendCounter_++;
    if (trendCounter_ % 100 == 0) { // 每100次更新改变一次趋势
        trend_ = generateRandomValue(-0.1, 0.1);  // 趋势变化范围±0.1
    }
    temperature += trend_ * (trendCounter_ % 100);  // 累积趋势变化
    
    // 第四步：添加昼夜变化（模拟真实环境）
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    int hour = tm.tm_hour;  // 获取当前小时
    
    // 模拟昼夜温差（白天稍热，晚上稍冷）
    // 使用正弦函数，6点为基准，12小时为周期
    double dayNightVariation = 2.0 * sin((hour - 6) * M_PI / 12.0);
    temperature += dayNightVariation;
    
    // 第五步：限制温度在合理范围内
    temperature = std::max(minValue_, std::min(maxValue_, temperature));
    
    return DataPoint(DataType::TEMPERATURE, temperature, "°C", name_);
}

// ==================== PressureSimulator 压力模拟器 ====================

/**
 * @brief 压力模拟器构造函数
 * @details 初始化压力模拟器的特定参数
 * @param name const String& 模拟器名称
 * 
 * 压力模拟器特点：
 * - 基础压力：1.0 MPa
 * - 噪声幅度：±0.05 MPa
 * - 更新间隔：1秒
 * - 数值范围：0.8 - 1.2 MPa
 */
PressureSimulator::PressureSimulator(const String& name)
    : DataSimulator(name)
    , basePressure_(1.0)                  // 基础压力1.0 MPa
    , noise_(0.05)                        // 随机噪声幅度±0.05 MPa
    , cycleCounter_(0) {                  // 周期计数器
    
    setMinValue(0.8);                     // 设置最小压力0.8 MPa
    setMaxValue(1.2);                     // 设置最大压力1.2 MPa
    setUpdateInterval(Duration(1000));    // 1秒更新一次
}

/**
 * @brief 生成压力数据点
 * @details 生成包含噪声、周期变化和波动的压力数据
 * @return DataPoint 压力数据点
 * 
 * 压力变化模式：
 * 1. 基础压力：1.0 MPa
 * 2. 随机噪声：±0.05 MPa的随机波动
 * 3. 周期变化：模拟设备运行周期的正弦变化
 * 4. 压力波动：偶尔的较大压力变化
 */
DataPoint PressureSimulator::generateData() {
    // 第一步：设置基础压力
    double pressure = basePressure_;
    
    // 第二步：添加随机噪声
    pressure += generateRandomValue(-noise_, noise_);
    
    // 第三步：添加周期性变化（模拟设备运行周期）
    cycleCounter_++;
    double cycleVariation = 0.02 * sin(cycleCounter_ * 0.1);  // 周期变化幅度±0.02 MPa
    pressure += cycleVariation;
    
    // 第四步：添加压力波动（模拟真实工业环境）
    if (cycleCounter_ % 50 == 0) {  // 每50次更新添加一次波动
        pressure += generateRandomValue(-0.05, 0.05);  // 波动幅度±0.05 MPa
    }
    
    // 第五步：限制压力在合理范围内
    pressure = std::max(minValue_, std::min(maxValue_, pressure));
    
    return DataPoint(DataType::PRESSURE, pressure, "MPa", name_);
}

// ==================== FlowSimulator 流量模拟器 ====================

/**
 * @brief 流量模拟器构造函数
 * @details 初始化流量模拟器的特定参数
 * @param name const String& 模拟器名称
 * 
 * 流量模拟器特点：
 * - 基础流量：100 L/min
 * - 波动幅度：±10 L/min
 * - 更新间隔：0.5秒（流量变化较快）
 * - 数值范围：80 - 120 L/min
 */
FlowSimulator::FlowSimulator(const String& name)
    : DataSimulator(name)
    , baseFlow_(100.0)                    // 基础流量100 L/min
    , fluctuation_(10.0)                  // 随机波动幅度±10 L/min
    , seasonalCounter_(0) {               // 季节性计数器
    
    setMinValue(80.0);                    // 设置最小流量80 L/min
    setMaxValue(120.0);                   // 设置最大流量120 L/min
    setUpdateInterval(Duration(500));     // 流量变化较快，0.5秒更新一次
}

/**
 * @brief 生成流量数据点
 * @details 生成包含波动、季节性变化和突然变化的流量数据
 * @return DataPoint 流量数据点
 * 
 * 流量变化模式：
 * 1. 基础流量：100 L/min
 * 2. 随机波动：±10 L/min的随机变化
 * 3. 季节性变化：缓慢的正弦变化
 * 4. 突然变化：模拟阀门开关等操作
 */
DataPoint FlowSimulator::generateData() {
    // 第一步：设置基础流量
    double flow = baseFlow_;
    
    // 第二步：添加随机波动
    flow += generateRandomValue(-fluctuation_, fluctuation_);
    
    // 第三步：添加季节性变化
    seasonalCounter_++;
    double seasonalVariation = 5.0 * sin(seasonalCounter_ * 0.01);  // 季节性变化幅度±5 L/min
    flow += seasonalVariation;
    
    // 第四步：模拟流量突然变化（模拟阀门开关等）
    if (seasonalCounter_ % 200 == 0) {  // 每200次更新添加一次突然变化
        flow += generateRandomValue(-20.0, 20.0);  // 突然变化幅度±20 L/min
    }
    
    // 第五步：限制流量在合理范围内
    flow = std::max(minValue_, std::min(maxValue_, flow));
    
    return DataPoint(DataType::FLOW, flow, "L/min", name_);
}

// ==================== StatusSimulator 状态模拟器 ====================

/**
 * @brief 状态模拟器构造函数
 * @details 初始化状态模拟器的特定参数
 * @param name const String& 模拟器名称
 * 
 * 状态模拟器特点：
 * - 状态值：0-4（正常、警告、错误、维护、离线）
 * - 更新间隔：5秒（状态变化较慢）
 * - 状态变化：每100次更新改变一次
 */
StatusSimulator::StatusSimulator(const String& name)
    : DataSimulator(name)
    , currentStatus_(0)                    // 当前状态值
    , statusCounter_(0) {                 // 状态计数器
    
    // 初始化状态值数组
    statusValues_ = {"正常", "警告", "错误", "维护", "离线"};
    
    setMinValue(0);                       // 设置最小状态值0
    setMaxValue(static_cast<double>(statusValues_.size() - 1));  // 设置最大状态值
    setUpdateInterval(Duration(5000));    // 状态变化较慢，5秒更新一次
}

/**
 * @brief 生成状态数据点
 * @details 生成设备状态数据，包含正常状态和异常状态
 * @return DataPoint 状态数据点
 * 
 * 状态变化逻辑：
 * 1. 每100次更新改变一次状态
 * 2. 偶尔出现异常状态（避免总是"正常"）
 * 3. 确保状态值在有效范围内
 */
DataPoint StatusSimulator::generateData() {
    // 第一步：状态变化逻辑
    statusCounter_++;
    
    // 每100次更新改变一次状态
    if (statusCounter_ % 100 == 0) {
        currentStatus_ = static_cast<int>(generateRandomValue(0, statusValues_.size()));
    }
    
    // 偶尔出现异常状态（避免总是"正常"）
    if (statusCounter_ % 500 == 0) {
        currentStatus_ = static_cast<int>(generateRandomValue(1, statusValues_.size())); // 避免总是"正常"
    }
    
    // 第二步：确保状态值在有效范围内
    currentStatus_ = std::max(0, std::min(static_cast<int>(statusValues_.size() - 1), currentStatus_));
    
    return DataPoint(DataType::STATUS, static_cast<double>(currentStatus_), "状态码", name_);
}

// ==================== CompositeSimulator 复合模拟器 ====================

/**
 * @brief 复合模拟器构造函数
 * @details 初始化复合模拟器，用于管理多个不同类型的模拟器
 * @param name const String& 模拟器名称
 */
CompositeSimulator::CompositeSimulator(const String& name)
    : name_(name) {
}

/**
 * @brief 添加模拟器到复合模拟器
 * @details 添加单个模拟器并建立数据类型映射
 * @param simulator std::shared_ptr<DataSimulator> 要添加的模拟器
 * 
 * 添加流程：
 * 1. 验证模拟器指针有效性
 * 2. 添加到模拟器列表
 * 3. 根据名称推断数据类型并建立映射
 */
void CompositeSimulator::addSimulator(std::shared_ptr<DataSimulator> simulator) {
    // 验证模拟器指针有效性
    if (!simulator) {
        return;
    }
    
    // 添加到模拟器列表
    simulators_.push_back(simulator);
    
    // 根据模拟器名称推断数据类型并建立映射
    String simName = simulator->getName();
    if (simName.find("Temperature") != String::npos || simName.find("温度") != String::npos) {
        typeMap_[DataType::TEMPERATURE] = simulator;      // 温度类型映射
    } else if (simName.find("Pressure") != String::npos || simName.find("压力") != String::npos) {
        typeMap_[DataType::PRESSURE] = simulator;         // 压力类型映射
    } else if (simName.find("Flow") != String::npos || simName.find("流量") != String::npos) {
        typeMap_[DataType::FLOW] = simulator;             // 流量类型映射
    } else if (simName.find("Status") != String::npos || simName.find("状态") != String::npos) {
        typeMap_[DataType::STATUS] = simulator;           // 状态类型映射
    }
    
    std::cout << "模拟器 " << simName << " 已添加到复合模拟器 " << name_ << std::endl;
}

/**
 * @brief 生成所有模拟器的数据
 * @details 调用所有模拟器生成数据点
 * @return std::vector<DataPoint> 所有模拟器的数据点集合
 * 
 * 生成流程：
 * 1. 遍历所有模拟器
 * 2. 调用每个模拟器的generateData()方法
 * 3. 异常处理和错误记录
 */
std::vector<DataPoint> CompositeSimulator::generateAllData() {
    std::vector<DataPoint> allData;
    
    // 遍历所有模拟器，生成数据
    for (auto& simulator : simulators_) {
        try {
            DataPoint data = simulator->generateData();
            allData.push_back(data);
        } catch (const std::exception& e) {
            // 异常处理：记录错误但继续处理其他模拟器
            std::cerr << "模拟器 " << simulator->getName() << " 生成数据失败: " << e.what() << std::endl;
        }
    }
    
    return allData;
}

/**
 * @brief 根据数据类型生成数据
 * @details 根据指定的数据类型调用对应的模拟器
 * @param type DataType 数据类型
 * @return DataPoint 指定类型的数据点
 * 
 * 查找流程：
 * 1. 在类型映射中查找对应类型的模拟器
 * 2. 如果找到，调用对应模拟器生成数据
 * 3. 如果没找到，返回默认数据
 */
DataPoint CompositeSimulator::generateData(DataType type) {
    // 在类型映射中查找对应类型的模拟器
    auto it = typeMap_.find(type);
    if (it != typeMap_.end()) {
        return it->second->generateData();  // 调用对应模拟器生成数据
    }
    
    // 如果没有找到对应类型的模拟器，返回默认数据
    std::cerr << "未找到类型 " << static_cast<int>(type) << " 的模拟器，返回默认数据" << std::endl;
    return DataPoint(type, 0.0, "N/A", "Default");
}

} // namespace plc
