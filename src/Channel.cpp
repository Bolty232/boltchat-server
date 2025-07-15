#include "Channel.h"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <vector>

Channel::Channel(std::string name)
    : name_(std::move(name)) {}

const std::string& Channel::getName() const {
    return name_;
}

size_t Channel::getMemberCount() const {
    std::lock_guard<std::mutex> lock(members_mutex_);
    return members_.size();
}

std::vector<std::string> Channel::getMemberNicknames() const {
    std::lock_guard<std::mutex> lock(members_mutex_);
    std::vector<std::string> nicknames;
    nicknames.reserve(members_.size());
    for (const auto& member : members_) {
        nicknames.push_back(member->getNickname());
    }
    return nicknames;
}

void Channel::addClient(std::shared_ptr<Client> client) {
    std::lock_guard<std::mutex> lock(members_mutex_);
    members_.insert(client);
    std::clog << "Client " << client->getNickname() << " joined channel " << name_ << std::endl;
}

void Channel::removeClient(std::shared_ptr<Client> client) {
    std::lock_guard<std::mutex> lock(members_mutex_);
    members_.erase(client);
    std::clog << "Client " << client->getNickname() << " left channel " << name_ << std::endl;
}

void Channel::broadcastMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(members_mutex_);
    std::string formatted_message = message + "\n";
    for (const auto& member : members_) {
        member->pushMessageToQueue(formatted_message);
    }
}