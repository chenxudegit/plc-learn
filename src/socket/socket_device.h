#ifndef SOCKET_DEVICE_H
#define SOCKET_DEVICE_H

#include "device.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <queue>

namespace plc {

// Socket消息结构
struct SocketMessage {
    String type;
    String data;
    TimePoint timestamp;
    
    SocketMessage(const String& t, const String& d)
        : type(t), data(d), timestamp(std::chrono::system_clock::now()) {}
};

// Socket设备类
class SocketDevice : public Device {
public:
    explicit SocketDevice(const DeviceConfig& config);
    virtual ~SocketDevice();
    
    // 重写基类方法
    ErrorCode initialize() override;
    ErrorCode cleanup() override;
    void runLoop() override;
    
    // Socket特定方法
    bool sendMessage(const SocketMessage& message);
    void broadcastData(const DataPoint& data);
    
    // 模拟数据生成
    void generateSimulatedData();
    
private:
    // 初始化
    void initializeCustomData();
    
    // Socket服务器相关
    bool startSocketServer();
    void stopSocketServer();
    void acceptConnections();
    void handleClient(int clientSocket);
    void broadcastToClients(const String& message);
    
    // 客户端请求处理
    void handleClientRequest(int clientSocket, const String& request);
    void sendCurrentDataToClient(int clientSocket);
    void sendStatusToClient(int clientSocket);
    void sendErrorToClient(int clientSocket, const String& error);
    
    // 数据广播
    void broadcastGeneratedData();
    
    // 数据模拟
    void simulateCounter();
    void simulateStatus();
    void simulateCustomData();
    
    // 成员变量
    int serverSocket_;
    std::vector<int> clientSockets_;
    std::queue<SocketMessage> messageQueue_;
    
    // 模拟参数
    int counterValue_;
    String statusValue_;
    std::vector<String> customData_;
    
    // 时间控制
    TimePoint lastUpdate_;
    Duration updateInterval_;
    
    // 线程安全
    std::mutex clientsMutex_;
    std::mutex queueMutex_;
};

} // namespace plc

#endif // SOCKET_DEVICE_H
