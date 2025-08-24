#pragma once

#include "common.h"
#include <sqlite3.h>
#include <string>
#include <vector>

namespace plc {

// 数据库管理器类
class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();
    
    // 基本操作
    bool initialize();
    void cleanup();
    
    // 数据操作
    bool insertDataPoint(const DataPoint& data);
    bool insertDeviceStatus(const String& deviceName, DeviceStatus status);
    
    // 数据查询
    std::vector<DataPoint> getDeviceData(const String& deviceName, 
                                        const TimePoint& startTime, 
                                        const TimePoint& endTime);
    
    DeviceStatus getDeviceStatus(const String& deviceName);
    
    // 统计信息
    size_t getDataPointCount(const String& deviceName);
    TimePoint getLastUpdateTime(const String& deviceName);
    
private:
    // 数据库连接
    sqlite3* db_;
    String dbPath_;
    
    // 初始化数据库表
    bool createTables();
    
    // 辅助方法
    bool executeQuery(const String& sql);
    String timePointToString(const TimePoint& time);
    TimePoint stringToTimePoint(const String& timeStr);
};

} // namespace plc

