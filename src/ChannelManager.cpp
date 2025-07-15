#include "ChannelManager.h"
#include <algorithm>
#include <iostream>

ChannelManager::ChannelManager(int maxChannels)
    : maxChannels_(maxChannels) {}

bool ChannelManager::isValidChannelName(const std::string& channelName) const {
    if (channelName.empty() || channelName.length() > 50) {
        return false;
    }
    if (channelName.find(' ') != std::string::npos || channelName.find(',') != std::string::npos) {
        return false;
    }
    if (channelName[0] != '#') {
        return false;
    }
    return std::all_of(channelName.begin() + 1, channelName.end(),
        [](char c) { return std::isprint(c) && !std::isspace(c); });
}

bool ChannelManager::canCreateNewChannel_UNLOCKED() const {
    return static_cast<int>(channels_.size()) < maxChannels_;
}

bool ChannelManager::createChannel_UNLOCKED(const std::string& channelName) {
    if (!isValidChannelName(channelName) || channels_.count(channelName) > 0 || !canCreateNewChannel_UNLOCKED()) {
        return false;
    }
    channels_[channelName] = std::make_unique<Channel>(channelName);
    return true;
}

bool ChannelManager::createChannel(const std::string& channelName) {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    return createChannel_UNLOCKED(channelName);
}

bool ChannelManager::removeChannel(const std::string& channelName) {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    auto it = channels_.find(channelName);
    if (it == channels_.end()) {
        return false;
    }
    channels_.erase(it);
    return true;
}

bool ChannelManager::channelExists(const std::string& channelName) const {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    return channels_.count(channelName) > 0;
}

std::optional<std::reference_wrapper<Channel>> ChannelManager::getChannel(const std::string& channelName) {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    auto it = channels_.find(channelName);
    if (it != channels_.end()) {
        return std::ref(*it->second);
    }
    return std::nullopt;
}

std::optional<std::reference_wrapper<const Channel>> ChannelManager::getChannel(const std::string& channelName) const {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    auto it = channels_.find(channelName);
    if (it != channels_.end()) {
        return std::cref(*it->second);
    }
    return std::nullopt;
}

bool ChannelManager::joinChannel(std::shared_ptr<Client> client, const std::string& channelName) {
    if (!client || !isValidChannelName(channelName)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(channels_mutex_);
    if (channels_.count(channelName) == 0) {
        if (!createChannel_UNLOCKED(channelName)) {
            return false;
        }
    }
    channels_.at(channelName)->addClient(client);
    client->joinChannel(channelName);
    return true;
}

bool ChannelManager::leaveChannel(std::shared_ptr<Client> client, const std::string& channelName) {
    if (!client) return false;
    std::lock_guard<std::mutex> lock(channels_mutex_);
    auto it = channels_.find(channelName);
    if (it == channels_.end()) {
        return false;
    }
    it->second->removeClient(client);
    client->leaveChannel(channelName);
    return true;
}

void ChannelManager::removeClientFromAllChannels(std::shared_ptr<Client> client) {
    if (!client) return;
    const auto channelSet = client->getJoinedChannels();
    std::vector<std::string> clientChannels(channelSet.begin(), channelSet.end());
    for (const auto& channelName : clientChannels) {
        leaveChannel(client, channelName);
    }
}

void ChannelManager::broadcastToChannel(const std::string& channelName, const std::string& message) {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    auto it = channels_.find(channelName);
    if (it != channels_.end()) {
        it->second->broadcastMessage(message);
    }
}

void ChannelManager::broadcastToAllChannels(const std::string& message) {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    for (const auto& [name, channel] : channels_) {
        channel->broadcastMessage(message);
    }
}

std::vector<std::string> ChannelManager::getChannelList() const {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    std::vector<std::string> channelNames;
    channelNames.reserve(channels_.size());
    for (const auto& [channelName, channel] : channels_) {
        channelNames.push_back(channelName);
    }
    std::sort(channelNames.begin(), channelNames.end());
    return channelNames;
}

size_t ChannelManager::getChannelCount() const {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    return channels_.size();
}

size_t ChannelManager::getChannelMemberCount(const std::string& channelName) const {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    auto it = channels_.find(channelName);
    if (it != channels_.end()) {
        return it->second->getMemberCount();
    }
    return 0;
}

std::vector<std::string> ChannelManager::getClientChannels(std::shared_ptr<Client> client) const {
    if (!client) {
        return {};
    }
    const auto& channelSet = client->getJoinedChannels();
    std::vector<std::string> channelVector(channelSet.begin(), channelSet.end());
    std::sort(channelVector.begin(), channelVector.end());
    return channelVector;
}

int ChannelManager::getMaxChannels() const {
    return maxChannels_;
}