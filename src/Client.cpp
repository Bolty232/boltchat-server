#include "Client.h"

Client::Client(int socket)
    : socket_(socket), nickname_("guest" + std::to_string(socket)) {}

int Client::getSocket() const {
    return socket_;
}

const std::string& Client::getNickname() const {
    return nickname_;
}

void Client::setNickname(const std::string& name) {
    nickname_ = name;
}

const std::string& Client::getActiveChannel() const {
    return active_channel_;
}

void Client::setActiveChannel(const std::string& channelName) {
    active_channel_ = channelName;
}

void Client::joinChannel(const std::string& channelName) {
    joined_channels_.insert(channelName);
}

void Client::leaveChannel(const std::string& channelName) {
    joined_channels_.erase(channelName);
    if (active_channel_ == channelName) {
        active_channel_.clear();
    }
}

const std::unordered_set<std::string>& Client::getJoinedChannels() const {
    return joined_channels_;
}

void Client::appendToBuffer(const char* data, size_t length) {
    readBuffer_.append(data, length);
}

std::string& Client::getReadBuffer() {
    return readBuffer_;
}

void Client::pushMessageToQueue(const std::string& message) {
    std::lock_guard<std::mutex> lock(output_mutex_);
    output_queue_.push(message);
}

std::string Client::getNextMessageFromQueue() {
    std::lock_guard<std::mutex> lock(output_mutex_);
    if (output_queue_.empty()) {
        return "";
    }
    return output_queue_.front();
}

void Client::popMessageFromQueue() {
    std::lock_guard<std::mutex> lock(output_mutex_);
    if (!output_queue_.empty()) {
        output_queue_.pop();
    }
}