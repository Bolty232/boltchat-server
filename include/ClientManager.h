#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>
#include <atomic>
#include "Client.h"

class ClientManager {
public:
    explicit ClientManager(int maxClients);

    bool addClient(std::shared_ptr<Client> client);
    bool removeClient(std::shared_ptr<Client> client);
    bool clientExists(std::shared_ptr<Client> client) const;
    bool clientExistsByNickname(const std::string& nickname) const;
    bool updateClientNickname(std::shared_ptr<Client> client, const std::string& newNickname);
    bool isValidNickname(const std::string& nickname) const;

    std::shared_ptr<Client> getClientByNickname(const std::string& nickname);
    std::vector<std::shared_ptr<Client>> getAllClients() const;

    void broadcastMessage(const std::string& message, std::shared_ptr<Client> sender = nullptr);
    void sendMessageToClient(std::shared_ptr<Client> client, const std::string& message);

    size_t getClientCount() const;
    size_t getTotalConnectionsCount() const;
    void incrementTotalConnections();
    int getMaxClients() const;
    bool canAcceptNewConnection() const;

    void setOnClientAddedCallback(std::function<void(std::shared_ptr<Client>)> callback);
    void setOnClientRemovedCallback(std::function<void(std::shared_ptr<Client>)> callback);

private:
    int maxClients_;
    std::unordered_set<std::shared_ptr<Client>> clients_;
    std::unordered_map<std::string, std::shared_ptr<Client>> nicknameToClient_;

    std::atomic<size_t> totalConnections_{0};

    std::function<void(std::shared_ptr<Client>)> onClientAddedCallback_;
    std::function<void(std::shared_ptr<Client>)> onClientRemovedCallback_;

    mutable std::mutex clients_mutex_;
};

#endif // CLIENTMANAGER_H