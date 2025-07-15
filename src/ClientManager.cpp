#include "ClientManager.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <algorithm>

ClientManager::ClientManager(int maxClients)
    : maxClients_(maxClients), totalConnections_(0) {}

bool ClientManager::addClient(std::shared_ptr<Client> client) {
    if (!client) return false;
    std::lock_guard<std::mutex> lock(clients_mutex_);
    if (clients_.size() >= static_cast<size_t>(maxClients_)) {
        return false;
    }
    auto [it, success] = clients_.insert(client);
    if (success) {
        nicknameToClient_[client->getNickname()] = client;
        if (onClientAddedCallback_) {
            onClientAddedCallback_(client);
        }
    }
    return success;
}

bool ClientManager::removeClient(std::shared_ptr<Client> client) {
    if (!client) return false;
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients_.find(client);
    if (it != clients_.end()) {
        int socket = client->getSocket();
        shutdown(socket, SHUT_RDWR);
        close(socket);
        nicknameToClient_.erase(client->getNickname());
        clients_.erase(it);
        if (onClientRemovedCallback_) {
            onClientRemovedCallback_(client);
        }
        return true;
    }
    return false;
}

bool ClientManager::clientExists(std::shared_ptr<Client> client) const {
    if (!client) return false;
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.find(client) != clients_.end();
}

bool ClientManager::clientExistsByNickname(const std::string& nickname) const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return nicknameToClient_.find(nickname) != nicknameToClient_.end();
}

std::shared_ptr<Client> ClientManager::getClientByNickname(const std::string& nickname) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = nicknameToClient_.find(nickname);
    if (it != nicknameToClient_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Client>> ClientManager::getAllClients() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    std::vector<std::shared_ptr<Client>> clientList;
    clientList.reserve(clients_.size());
    for (const auto& client : clients_) {
        clientList.push_back(client);
    }
    return clientList;
}

void ClientManager::broadcastMessage(const std::string& message, std::shared_ptr<Client> sender) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    std::string formatted_message;
    if (sender) {
        formatted_message = "<" + sender->getNickname() + "> " + message;
    } else {
        formatted_message = message;
    }
    for (const auto& client : clients_) {
        if (client != sender) {
            client->pushMessageToQueue(formatted_message + "\n");
        }
    }
}

void ClientManager::sendMessageToClient(std::shared_ptr<Client> client, const std::string& message) {
    if (!client) return;
    std::string formatted_message = message;
    if (formatted_message.back() != '\n') {
        formatted_message += "\n";
    }
    client->pushMessageToQueue(formatted_message);
}

size_t ClientManager::getClientCount() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}

size_t ClientManager::getTotalConnectionsCount() const {
    return totalConnections_.load();
}

void ClientManager::incrementTotalConnections() {
    totalConnections_++;
}

int ClientManager::getMaxClients() const {
    return maxClients_;
}

bool ClientManager::canAcceptNewConnection() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size() < static_cast<size_t>(maxClients_);
}

void ClientManager::setOnClientAddedCallback(std::function<void(std::shared_ptr<Client>)> callback) {
    onClientAddedCallback_ = std::move(callback);
}

void ClientManager::setOnClientRemovedCallback(std::function<void(std::shared_ptr<Client>)> callback) {
    onClientRemovedCallback_ = std::move(callback);
}

bool ClientManager::isValidNickname(const std::string& nickname) const {
    if (nickname.empty() || nickname.length() > 32) {
        return false;
    }
    return std::all_of(nickname.begin(), nickname.end(),
        [](char c) { return std::isalnum(c) || c == '_'; });
}

bool ClientManager::updateClientNickname(std::shared_ptr<Client> client, const std::string& newNickname) {
    if (!client || !isValidNickname(newNickname)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(clients_mutex_);
    if (nicknameToClient_.find(newNickname) != nicknameToClient_.end()) {
        return false;
    }
    nicknameToClient_.erase(client->getNickname());
    std::string oldNickname = client->getNickname();
    client->setNickname(newNickname);
    nicknameToClient_[newNickname] = client;
    return true;
}