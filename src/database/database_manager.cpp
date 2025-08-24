/**
 * @file database_manager.cpp
 * @brief 数据库管理器实现文件
 * @details 负责SQLite数据库的管理，包括数据存储、查询、表创建和维护
 * @author 工业设备数据模拟系统开发团队
 * @date 2025
 * @version 1.0.0
 */

#include "../../include/database_manager.h"
#include "../../include/common.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace plc {

/**
 * @brief 数据库管理器构造函数
 * @details 初始化数据库管理器的基本属性
 * 
 * 初始化内容：
 * - 设置数据库指针为nullptr
 * - 设置默认数据库路径为"data/plc_data.db"
 */
DatabaseManager::DatabaseManager()
    : db_(nullptr)                           // 初始化数据库指针为nullptr
    , dbPath_("data/plc_data.db") {         // 设置默认数据库路径
    
    std::cout << "数据库管理器已创建" << std::endl;
}

/**
 * @brief 数据库管理器析构函数
 * @details 确保在销毁前正确清理数据库资源
 */
DatabaseManager::~DatabaseManager() {
    cleanup();  // 调用清理函数
}

/**
 * @brief 初始化数据库管理器
 * @details 执行数据库的初始化操作
 * @return bool 初始化是否成功
 * 
 * 初始化流程：
 * 1. 创建数据目录
 * 2. 打开SQLite数据库连接
 * 3. 创建必要的数据表
 * 4. 返回初始化结果
 */
bool DatabaseManager::initialize() {
    try {
        std::cout << "初始化数据库管理器..." << std::endl;
        
        // 第一步：创建数据目录（如果不存在）
        system("mkdir -p data");
        
        // 第二步：打开SQLite数据库连接
        int rc = sqlite3_open(dbPath_.c_str(), &db_);
        if (rc != SQLITE_OK) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        std::cout << "数据库连接成功: " << dbPath_ << std::endl;
        
        // 第三步：创建必要的数据表
        if (!createTables()) {
            std::cerr << "创建数据表失败" << std::endl;
            return false;
        }
        
        std::cout << "数据库管理器初始化成功" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "数据库初始化异常: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief 清理数据库资源
 * @details 关闭数据库连接并清理相关资源
 */
void DatabaseManager::cleanup() {
    if (db_) {
        sqlite3_close(db_);  // 关闭SQLite数据库连接
        db_ = nullptr;       // 重置数据库指针
        std::cout << "数据库连接已关闭" << std::endl;
    }
}

/**
 * @brief 创建数据库表
 * @details 创建系统所需的所有数据表
 * @return bool 表创建是否成功
 * 
 * 创建的表：
 * - device_data: 设备数据表
 * - device_status: 设备状态表
 * - device_config: 设备配置表
 */
bool DatabaseManager::createTables() {
    // 第一步：创建设备数据表
    const char* createDeviceDataTable = R"(
        CREATE TABLE IF NOT EXISTS device_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            device_name TEXT NOT NULL,
            data_type TEXT NOT NULL,
            value REAL NOT NULL,
            unit TEXT,
            source TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    if (!executeQuery(createDeviceDataTable)) {
        return false;
    }
    
    // 第二步：创建设备状态表
    const char* createDeviceStatusTable = R"(
        CREATE TABLE IF NOT EXISTS device_status (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            device_name TEXT NOT NULL,
            status TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    if (!executeQuery(createDeviceStatusTable)) {
        return false;
    }
    
    // 第三步：创建设备配置表
    const char* createDeviceConfigTable = R"(
        CREATE TABLE IF NOT EXISTS device_config (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_name TEXT UNIQUE NOT NULL,
            device_type TEXT NOT NULL,
            address TEXT,
            port INTEGER,
            config_json TEXT,
            enabled BOOLEAN DEFAULT 1,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    if (!executeQuery(createDeviceConfigTable)) {
        return false;
    }
    
    // 创建索引以提高查询性能
    executeQuery("CREATE INDEX IF NOT EXISTS idx_device_data_timestamp ON device_data(timestamp);");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_device_data_device_name ON device_data(device_name);");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_device_status_device_name ON device_status(device_name);");
    
    std::cout << "数据库表创建成功" << std::endl;
    return true;
}

bool DatabaseManager::insertDataPoint(const DataPoint& data) {
    if (!db_) {
        std::cerr << "数据库未初始化" << std::endl;
        return false;
    }
    
    try {
        const char* sql = R"(
            INSERT INTO device_data (timestamp, device_name, data_type, value, unit, source)
            VALUES (?, ?, ?, ?, ?, ?);
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            std::cerr << "准备SQL语句失败: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        // 绑定参数
        String timestampStr = timePointToString(data.timestamp);
        String dataTypeStr = Utils::dataTypeToString(data.type);
        
        sqlite3_bind_text(stmt, 1, timestampStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, data.source.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, dataTypeStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 4, data.value);
        sqlite3_bind_text(stmt, 5, data.unit.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, data.source.c_str(), -1, SQLITE_STATIC);
        
        // 执行语句
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            std::cerr << "插入数据失败: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "插入数据异常: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::insertDeviceStatus(const String& deviceName, DeviceStatus status) {
    if (!db_) {
        std::cerr << "数据库未初始化" << std::endl;
        return false;
    }
    
    try {
        const char* sql = R"(
            INSERT INTO device_status (timestamp, device_name, status)
            VALUES (?, ?, ?);
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            std::cerr << "准备SQL语句失败: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        // 绑定参数
        String timestampStr = timePointToString(Utils::getCurrentTime());
        String statusStr;
        switch (status) {
            case DeviceStatus::OFFLINE: statusStr = "offline"; break;
            case DeviceStatus::ONLINE: statusStr = "online"; break;
            case DeviceStatus::ERROR: statusStr = "error"; break;
            case DeviceStatus::MAINTENANCE: statusStr = "maintenance"; break;
        }
        
        sqlite3_bind_text(stmt, 1, timestampStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, deviceName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, statusStr.c_str(), -1, SQLITE_STATIC);
        
        // 执行语句
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            std::cerr << "插入设备状态失败: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "插入设备状态异常: " << e.what() << std::endl;
        return false;
    }
}

std::vector<DataPoint> DatabaseManager::getDeviceData(const String& deviceName, 
                                                      const TimePoint& startTime, 
                                                      const TimePoint& endTime) {
    std::vector<DataPoint> results;
    
    if (!db_) {
        std::cerr << "数据库未初始化" << std::endl;
        return results;
    }
    
    try {
        const char* sql = R"(
            SELECT timestamp, data_type, value, unit, source
            FROM device_data
            WHERE device_name = ? AND timestamp >= ? AND timestamp <= ?
            ORDER BY timestamp DESC
            LIMIT 1000;
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            std::cerr << "准备SQL语句失败: " << sqlite3_errmsg(db_) << std::endl;
            return results;
        }
        
        // 绑定参数
        String startTimeStr = timePointToString(startTime);
        String endTimeStr = timePointToString(endTime);
        
        sqlite3_bind_text(stmt, 1, deviceName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, startTimeStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, endTimeStr.c_str(), -1, SQLITE_STATIC);
        
        // 执行查询
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            String timestampStr = (char*)sqlite3_column_text(stmt, 0);
            String dataTypeStr = (char*)sqlite3_column_text(stmt, 1);
            double value = sqlite3_column_double(stmt, 2);
            String unit = (char*)sqlite3_column_text(stmt, 3);
            String source = (char*)sqlite3_column_text(stmt, 4);
            
            DataPoint data;
            data.timestamp = stringToTimePoint(timestampStr);
            data.type = Utils::stringToDataType(dataTypeStr);
            data.value = value;
            data.unit = unit;
            data.source = source;
            
            results.push_back(data);
        }
        
        sqlite3_finalize(stmt);
        
    } catch (const std::exception& e) {
        std::cerr << "查询设备数据异常: " << e.what() << std::endl;
    }
    
    return results;
}

DeviceStatus DatabaseManager::getDeviceStatus(const String& deviceName) {
    if (!db_) {
        std::cerr << "数据库未初始化" << std::endl;
        return DeviceStatus::OFFLINE;
    }
    
    try {
        const char* sql = R"(
            SELECT status
            FROM device_status
            WHERE device_name = ?
            ORDER BY timestamp DESC
            LIMIT 1;
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            std::cerr << "准备SQL语句失败: " << sqlite3_errmsg(db_) << std::endl;
            return DeviceStatus::OFFLINE;
        }
        
        sqlite3_bind_text(stmt, 1, deviceName.c_str(), -1, SQLITE_STATIC);
        
        DeviceStatus status = DeviceStatus::OFFLINE;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            String statusStr = (char*)sqlite3_column_text(stmt, 0);
            if (statusStr == "online") status = DeviceStatus::ONLINE;
            else if (statusStr == "error") status = DeviceStatus::ERROR;
            else if (statusStr == "maintenance") status = DeviceStatus::MAINTENANCE;
        }
        
        sqlite3_finalize(stmt);
        return status;
        
    } catch (const std::exception& e) {
        std::cerr << "查询设备状态异常: " << e.what() << std::endl;
        return DeviceStatus::OFFLINE;
    }
}

size_t DatabaseManager::getDataPointCount(const String& deviceName) {
    if (!db_) {
        return 0;
    }
    
    try {
        const char* sql = R"(
            SELECT COUNT(*) FROM device_data WHERE device_name = ?;
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            std::cerr << "准备SQL语句失败: " << sqlite3_errmsg(db_) << std::endl;
            return 0;
        }
        
        sqlite3_bind_text(stmt, 1, deviceName.c_str(), -1, SQLITE_STATIC);
        
        size_t count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int64(stmt, 0);
        }
        
        sqlite3_finalize(stmt);
        return count;
        
    } catch (const std::exception& e) {
        std::cerr << "查询数据点数量异常: " << e.what() << std::endl;
        return 0;
    }
}

TimePoint DatabaseManager::getLastUpdateTime(const String& deviceName) {
    if (!db_) {
        return TimePoint{};
    }
    
    try {
        const char* sql = R"(
            SELECT timestamp FROM device_data 
            WHERE device_name = ? 
            ORDER BY timestamp DESC 
            LIMIT 1;
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            std::cerr << "准备SQL语句失败: " << sqlite3_errmsg(db_) << std::endl;
            return TimePoint{};
        }
        
        sqlite3_bind_text(stmt, 1, deviceName.c_str(), -1, SQLITE_STATIC);
        
        TimePoint lastTime{};
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            String timestampStr = (char*)sqlite3_column_text(stmt, 0);
            lastTime = stringToTimePoint(timestampStr);
        }
        
        sqlite3_finalize(stmt);
        return lastTime;
        
    } catch (const std::exception& e) {
        std::cerr << "查询最后更新时间异常: " << e.what() << std::endl;
        return TimePoint{};
    }
}

bool DatabaseManager::executeQuery(const String& sql) {
    if (!db_) {
        std::cerr << "数据库未初始化" << std::endl;
        return false;
    }
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL执行失败: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

String DatabaseManager::timePointToString(const TimePoint& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto tm = *std::gmtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

TimePoint DatabaseManager::stringToTimePoint(const String& timeStr) {
    std::istringstream iss(timeStr);
    std::tm tm = {};
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    auto time_t = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(time_t);
}

} // namespace plc
