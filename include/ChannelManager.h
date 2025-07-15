#ifndef CHANNELMANAGER_H
#define CHANNELMANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <optional>
#include <functional>

#include "Channel.h"
#include "Client.h"

class ChannelManager {
public:
    explicit ChannelManager(int maxChannels = 1000);
    ~ChannelManager() = default;

    ChannelManager(const ChannelManager&) = delete;
    ChannelManager& operator=(const ChannelManager&) = delete;

    bool createChannel(const std::string& channelName);
    bool removeChannel(const std::string& channelName);
    bool channelExists(const std::string& channelName) const;

    std::optional<std::reference_wrapper<Channel>> getChannel(const std::string& channelName);

    std::optional<std::reference_wrapper<const Channel>> getChannel(const std::string& channelName) const;

    bool joinChannel(std::shared_ptr<Client> client, const std::string& channelName);
    bool leaveChannel(std::shared_ptr<Client> client, const std::string& channelName);
    void removeClientFromAllChannels(std::shared_ptr<Client> client);

    void broadcastToChannel(const std::string& channelName, const std::string& message);
    void broadcastToAllChannels(const std::string& message);


    std::vector<std::string> getChannelList() const;

    std::vector<std::string> getClientChannels(std::shared_ptr<Client> client) const;

    size_t getChannelCount() const;
    size_t getChannelMemberCount(const std::string& channelName) const;
    int getMaxChannels() const;

private:
    bool isValidChannelName(const std::string& channelName) const;
    bool canCreateNewChannel_UNLOCKED() const;
    bool createChannel_UNLOCKED(const std::string& channelName);

    std::unordered_map<std::string, std::unique_ptr<Channel>> channels_;
    mutable std::mutex channels_mutex_;

    const int maxChannels_;
};

#endif //CHANNELMANAGER_H