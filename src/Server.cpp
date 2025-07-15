#include "Server.h"
#include "ConfigReader.h"
#include "Client.h"
#include "ChannelManager.h"
#include "ClientManager.h"
#include "MessageManager.h"
#include <iostream>
#include <system_error>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

namespace {
    struct ServerConfig {
        static constexpr int MIN_PORT = 1024;
        static constexpr int MAX_PORT = 65535;
        static constexpr int MIN_USERS = 1;
        static constexpr int MAX_USERS = 10000;
        static constexpr int MIN_CHANNELS = 0;
        static constexpr int MAX_CHANNELS = 1000;
        static constexpr int DEFAULT_PORT = 4040;
        static constexpr int DEFAULT_MAX_USERS = 2000;
        static constexpr int DEFAULT_MAX_CHANNELS = 1000;
        static constexpr unsigned int DEFAULT_THREAD_POOL_SIZE = 10;
        static constexpr size_t RECV_BUFFER_SIZE = 4096;
        static const std::string DEFAULT_SERVER_NAME;
        static const std::string DEFAULT_MOTD;
    };
    const std::string ServerConfig::DEFAULT_SERVER_NAME = "Test-Server";
    const std::string ServerConfig::DEFAULT_MOTD = "Welcome to test Server!";
}

class ServerError : public std::runtime_error {
public:
    explicit ServerError(const std::string& message) : std::runtime_error(message) {}
};

Server::Server()
    : port_(ServerConfig::DEFAULT_PORT),
      maxUsers_(ServerConfig::DEFAULT_MAX_USERS),
      maxChannels_(ServerConfig::DEFAULT_MAX_CHANNELS),
      servername_(ServerConfig::DEFAULT_SERVER_NAME),
      motd_(ServerConfig::DEFAULT_MOTD),
      listeningSocket_(-1),
      threadPool_(ServerConfig::DEFAULT_THREAD_POOL_SIZE) {
    config_ = {
        {"port", std::to_string(ServerConfig::DEFAULT_PORT)},
        {"maxchannels", std::to_string(ServerConfig::DEFAULT_MAX_CHANNELS)},
        {"maxusers", std::to_string(ServerConfig::DEFAULT_MAX_USERS)},
        {"servername", ServerConfig::DEFAULT_SERVER_NAME},
        {"motd", ServerConfig::DEFAULT_MOTD},
    };
    initializeManagers();
}

Server::Server(const std::string& configPath)
    : listeningSocket_(-1),
      threadPool_(ServerConfig::DEFAULT_THREAD_POOL_SIZE) {
    auto config = readConfig(configPath);
    if (!config) {
        throw ServerError("Failed to read configuration file: " + configPath);
    }
    try {
        config_.clear();
        for (const auto& [key, value] : config.value()) {
            config_[key] = value;
        }
        port_ = std::stoi(config_.at("port"));
        maxUsers_ = std::stoi(config_.at("maxusers"));
        maxChannels_ = std::stoi(config_.at("maxchannels"));
        servername_ = config_.at("servername");
        motd_ = config_.at("motd");
        validateConfig();
        initializeManagers();
    } catch (const std::out_of_range& e) {
        throw ServerError("Missing required configuration parameter");
    } catch (const std::invalid_argument& e) {
        throw ServerError("Invalid configuration parameter value");
    }
}

Server::~Server() {
    if (running_.load()) {
        stop();
    }
}

void Server::validateConfig() {
    if (port_ < ServerConfig::MIN_PORT || port_ > ServerConfig::MAX_PORT) {
        throw ServerError("Invalid port number.");
    }
    if (maxUsers_ < ServerConfig::MIN_USERS || maxUsers_ > ServerConfig::MAX_USERS) {
        throw ServerError("Invalid max users value");
    }
    if (maxChannels_ < ServerConfig::MIN_CHANNELS || maxChannels_ > ServerConfig::MAX_CHANNELS) {
        throw ServerError("Invalid max channels value");
    }
    if (servername_.empty()) {
        throw ServerError("Server name cannot be empty");
    }
}

void Server::initializeManagers() {
    channelManager_ = std::make_unique<ChannelManager>(maxChannels_);
    clientManager_ = std::make_unique<ClientManager>(maxUsers_);
    messageManager_ = std::make_unique<MessageManager>(*clientManager_, *channelManager_);
    messageManager_->setMotd(motd_);
    clientManager_->setOnClientRemovedCallback([this](std::shared_ptr<Client> client) {
        channelManager_->removeClientFromAllChannels(client);
    });
}

void Server::initializeSocket() {
    listeningSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listeningSocket_ == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to create socket");
    }
    int reuse = 1;
    if (setsockopt(listeningSocket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(listeningSocket_);
        throw std::system_error(errno, std::system_category(), "Failed to set SO_REUSEADDR");
    }
    int flags = fcntl(listeningSocket_, F_GETFL, 0);
    if (flags == -1) {
         close(listeningSocket_);
        throw std::system_error(errno, std::system_category(), "Failed to get socket flags");
    }
    if (fcntl(listeningSocket_, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(listeningSocket_);
        throw std::system_error(errno, std::system_category(), "Failed to set non-blocking");
    }
    sockaddr_in hint{};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port_);
    hint.sin_addr.s_addr = INADDR_ANY;
    if (bind(listeningSocket_, reinterpret_cast<sockaddr*>(&hint), sizeof(hint)) == -1) {
        close(listeningSocket_);
        throw std::system_error(errno, std::system_category(), "Failed to bind socket");
    }
    if (listen(listeningSocket_, SOMAXCONN) == -1) {
        close(listeningSocket_);
        throw std::system_error(errno, std::system_category(), "Failed to listen on socket");
    }
}

void Server::start() {
    try {
        validateConfig();
        initializeSocket();
        running_ = true;
        while (running_.load()) {
            if (!canAcceptNewConnection()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            int clientSocket = accept(listeningSocket_, nullptr, nullptr);
            if (clientSocket == -1) {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
            int flags = fcntl(clientSocket, F_GETFL, 0);
            fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
            auto newClient = std::make_shared<Client>(clientSocket);
            clientManager_->addClient(newClient);
            clientManager_->incrementTotalConnections();
            if (!threadPool_.enqueue([this, newClient]() { this->handleClient(newClient); })) {
                clientManager_->removeClient(newClient);
            }
        }
    } catch (const std::exception& e) {
        stop();
        throw;
    }
}

void Server::stop() {
    if (running_.exchange(false)) {
        if (listeningSocket_ != -1) {
            shutdown(listeningSocket_, SHUT_RDWR);
            close(listeningSocket_);
            listeningSocket_ = -1;
        }
        auto clients = clientManager_->getAllClients();
        for (const auto& client : clients) {
            clientManager_->removeClient(client);
        }
    }
}

void Server::handleClient(std::shared_ptr<Client> client) {
    try {
        messageManager_->sendServerMessage(client, "Welcome to " + servername_ + "!");
        messageManager_->sendServerMessage(client, "Type /help for a list of available commands.");
        const auto clientSocket = client->getSocket();
        std::vector<char> buffer(ServerConfig::RECV_BUFFER_SIZE);
        while (running_.load() && clientManager_->clientExists(client)) {
            const auto bytesReceived = recv(clientSocket, buffer.data(), buffer.size(), 0);
            if (bytesReceived > 0) {
                client->appendToBuffer(buffer.data(), bytesReceived);
                if (client->getReadBuffer().length() > MAX_CLIENT_BUFFER_SIZE) {
                    break;
                }
                size_t pos;
                while ((pos = client->getReadBuffer().find('\n')) != std::string::npos) {
                    std::string message = client->getReadBuffer().substr(0, pos);
                    client->getReadBuffer().erase(0, pos + 1);
                    if (!message.empty()) {
                        messageManager_->handleMessage(client, message);
                    }
                }
            } else if (bytesReceived == 0) {
                break;
            } else {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    break;
                }
            }
            processClientOutput(client);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    } catch (const std::exception& e) {
    }
    if (clientManager_->clientExists(client)) {
        clientManager_->removeClient(client);
    }
}

void Server::processClientOutput(std::shared_ptr<Client> client) {
    const auto clientSocket = client->getSocket();
    std::string message = client->getNextMessageFromQueue();
    while(!message.empty()) {
        ssize_t bytesSent = send(clientSocket, message.c_str(), message.length(), 0);
        if (bytesSent > 0) {
            client->popMessageFromQueue();
        } else {
             if (errno != EWOULDBLOCK && errno != EAGAIN) {
                clientManager_->removeClient(client);
            }
            break;
        }
        message = client->getNextMessageFromQueue();
    }
}

bool Server::canAcceptNewConnection() const {
    return clientManager_->canAcceptNewConnection();
}

ServerStats Server::getStats() const {
    return ServerStats{
        clientManager_->getClientCount(),
        clientManager_->getTotalConnectionsCount(),
        messageManager_->getReceivedBytesCount(),
        messageManager_->getSentBytesCount(),
        threadPool_.getActiveThreadCount(),
        threadPool_.getTaskCount()
    };
}

int Server::getPort() const { return port_; }
int Server::getMaxUsers() const { return maxUsers_; }
int Server::getMaxChannels() const { return maxChannels_; }
bool Server::isRunning() const { return running_.load(); }
std::string Server::getServerName() const { return servername_; }
std::string Server::getMOTD() const { return motd_; }