#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <unordered_set>
#include <vector>
#include <queue>
#include <mutex>

class Client {
public:
    explicit Client(int socket);

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    int getSocket() const;
    const std::string& getNickname() const;
    const std::unordered_set<std::string>& getJoinedChannels() const;
    std::string& getReadBuffer();
    const std::string& getActiveChannel() const;

    void setNickname(const std::string& name);
    void setActiveChannel(const std::string& channelName);
    void joinChannel(const std::string& channelName);
    void leaveChannel(const std::string& channelName);

    void appendToBuffer(const char* data, size_t length);

    void pushMessageToQueue(const std::string& message);
    std::string getNextMessageFromQueue();
    void popMessageFromQueue();

private:
    const int socket_;
    std::string nickname_;
    std::string readBuffer_;
    std::unordered_set<std::string> joined_channels_;
    std::string active_channel_;

    std::queue<std::string> output_queue_;
    mutable std::mutex output_mutex_;
};

#endif //CLIENT_H