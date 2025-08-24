/**
 * @file common.cpp
 * @brief 通用工具函数实现文件
 * @details 包含时间处理、数据类型转换、错误码转换等通用功能
 * @author 工业设备数据模拟系统开发团队
 * @date 2025
 * @version 1.0.0
 */

#include "../../include/common.h"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace plc {

/**
 * @brief 获取当前时间的字符串表示
 * @details 将当前系统时间转换为格式化的字符串，格式为 "YYYY-MM-DD HH:MM:SS"
 * @return String 格式化的时间字符串
 * 
 * 实现说明：
 * 1. 使用 std::chrono::system_clock 获取当前系统时间
 * 2. 将时间点转换为 time_t 类型
 * 3. 使用 std::localtime 转换为本地时间结构
 * 4. 使用 std::put_time 格式化为字符串
 */
String Utils::getCurrentTimeString() {
    // 获取当前系统时间点
    auto now = std::chrono::system_clock::now();
    // 将时间点转换为 time_t 类型，便于后续处理
    auto time_t = std::chrono::system_clock::to_time_t(now);
    // 转换为本地时间结构体
    auto tm = *std::localtime(&time_t);
    
    // 创建输出字符串流
    std::ostringstream oss;
    // 使用 put_time 格式化时间，格式：年-月-日 时:分:秒
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

/**
 * @brief 获取当前时间点
 * @details 返回当前系统时间的精确时间点，用于时间计算和比较
 * @return TimePoint 当前时间点
 * 
 * 实现说明：
 * 直接返回 std::chrono::system_clock::now() 的结果
 * 这个时间点可以用于计算时间差、设置超时等操作
 */
TimePoint Utils::getCurrentTime() {
    return std::chrono::system_clock::now();
}

/**
 * @brief 将数据类型枚举转换为字符串
 * @details 将内部的数据类型枚举值转换为可读的字符串表示
 * @param type DataType 数据类型枚举值
 * @return String 对应的字符串表示
 * 
 * 支持的数据类型：
 * - TEMPERATURE: 温度数据
 * - PRESSURE: 压力数据  
 * - FLOW: 流量数据
 * - STATUS: 状态数据
 * - CUSTOM: 自定义数据
 * - 其他: 返回 "unknown"
 */
String Utils::dataTypeToString(DataType type) {
    switch (type) {
        case DataType::TEMPERATURE: return "temperature";  // 温度数据类型
        case DataType::PRESSURE: return "pressure";        // 压力数据类型
        case DataType::FLOW: return "flow";                // 流量数据类型
        case DataType::STATUS: return "status";            // 状态数据类型
        case DataType::CUSTOM: return "custom";            // 自定义数据类型
        default: return "unknown";                         // 未知数据类型
    }
}

/**
 * @brief 将字符串转换为数据类型枚举
 * @details 将字符串表示的数据类型转换为内部枚举值
 * @param str const String& 数据类型的字符串表示
 * @return DataType 对应的数据类型枚举值
 * 
 * 转换规则：
 * - "temperature" -> TEMPERATURE
 * - "pressure" -> PRESSURE
 * - "flow" -> FLOW
 * - "status" -> STATUS
 * - "custom" -> CUSTOM
 * - 其他字符串 -> CUSTOM (默认)
 */
DataType Utils::stringToDataType(const String& str) {
    if (str == "temperature") return DataType::TEMPERATURE;  // 温度
    if (str == "pressure") return DataType::PRESSURE;        // 压力
    if (str == "flow") return DataType::FLOW;                // 流量
    if (str == "status") return DataType::STATUS;            // 状态
    if (str == "custom") return DataType::CUSTOM;            // 自定义
    return DataType::CUSTOM;                                 // 默认返回自定义类型
}

/**
 * @brief 将错误码转换为中文描述字符串
 * @details 将系统内部的错误码转换为用户友好的中文描述
 * @param code ErrorCode 错误码枚举值
 * @return String 中文错误描述
 * 
 * 错误码说明：
 * - SUCCESS: 操作成功
 * - CONNECTION_FAILED: 连接失败
 * - INVALID_DATA: 数据无效
 * - TIMEOUT: 操作超时
 * - UNKNOWN_ERROR: 未知错误
 */
String Utils::errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "成功";                    // 操作成功
        case ErrorCode::CONNECTION_FAILED: return "连接失败";      // 连接失败
        case ErrorCode::INVALID_DATA: return "无效数据";           // 数据无效
        case ErrorCode::TIMEOUT: return "超时";                    // 操作超时
        case ErrorCode::UNKNOWN_ERROR: return "未知错误";          // 未知错误
        default: return "未知错误码";                              // 未定义的错误码
    }
}

} // namespace plc
