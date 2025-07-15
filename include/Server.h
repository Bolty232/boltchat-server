#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <atomic>

#include "ThreadPool.h"
#include "ChannelManager.h"
#include "ClientManager.h"
#include "MessageManager.h"

struct ServerStats {
    size_t activeConnections;
    size_t totalConnections;
    size_t bytesReceived;
    size_t bytesSent;
    size_t activeThreads;
    size_t pendingTasks;
};

class Server {
public:
    Server();
    explicit Server(const std::string& configPath);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void start();
    void stop();

    bool isRunning() const;

    int getPort() const;
    int getMaxUsers() const;
    int getMaxChannels() const;
    std::string getServerName() const;
    std::string getMOTD() const;

    ServerStats getStats() const;

private:
    static constexpr size_t MAX_CLIENT_BUFFER_SIZE = 8192; // 8 KB

    int port_;
    int maxUsers_;
    int maxChannels_;
    std::string servername_;
    std::string motd_;
    std::unordered_map<std::string, std::string> config_;
    int listeningSocket_;

    std::unique_ptr<ChannelManager> channelManager_;
    std::unique_ptr<ClientManager> clientManager_;
    std::unique_ptr<MessageManager> messageManager_;

    ThreadPool threadPool_;
    std::atomic<bool> running_{false};


    void validateConfig();
    void initializeSocket();
    void initializeManagers();
    void handleClient(std::shared_ptr<Client> client);
    void processClientOutput(std::shared_ptr<Client> client);
    bool canAcceptNewConnection() const;
};

#endif //SERVER_H