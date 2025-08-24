#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>

namespace plc {

// 基本数据类型
using String = std::string;
using TimePoint = std::chrono::system_clock::time_point;
using Duration = std::chrono::milliseconds;

// 设备数据类型
enum class DataType {
    TEMPERATURE,
    PRESSURE,
    FLOW,
    STATUS,
    CUSTOM
};

// 数据点结构
struct DataPoint {
    TimePoint timestamp;
    DataType type;
    double value;
    String unit;
    String source;
    
    DataPoint() = default;
    DataPoint(DataType t, double v, const String& u, const String& s)
        : timestamp(std::chrono::system_clock::now())
        , type(t)
        , value(v)
        , unit(u)
        , source(s) {}
};

// 设备状态
enum class DeviceStatus {
    OFFLINE,
    ONLINE,
    ERROR,
    MAINTENANCE
};

// 配置结构
struct DeviceConfig {
    String name;
    String address;
    int port;
    Duration updateInterval;
    bool enabled;
    
    DeviceConfig() : port(0), updateInterval(Duration(1000)), enabled(true) {}
};

// 错误码
enum class ErrorCode {
    SUCCESS = 0,
    CONNECTION_FAILED = -1,
    INVALID_DATA = -2,
    TIMEOUT = -3,
    UNKNOWN_ERROR = -999
};

// 工具函数
class Utils {
public:
    static String getCurrentTimeString();
    static TimePoint getCurrentTime();
    static String dataTypeToString(DataType type);
    static DataType stringToDataType(const String& str);
    static String errorCodeToString(ErrorCode code);
};

} // namespace plc

