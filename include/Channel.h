#ifndef CHANNEL_H
#define CHANNEL_H

#include <string>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <vector>

#include "Client.h"

class Channel {
public:
    explicit Channel(std::string name);

    size_t getMemberCount() const;
    std::vector<std::string> getMemberNicknames() const;

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    const std::string& getName() const;

    void addClient(std::shared_ptr<Client> client);
    void removeClient(std::shared_ptr<Client> client);

    void broadcastMessage(const std::string& message);

private:
    std::string name_;
    std::unordered_set<std::shared_ptr<Client>> members_;
    mutable std::mutex members_mutex_;
};

#endif //CHANNEL_H