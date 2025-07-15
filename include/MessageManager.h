#ifndef MESSAGEMANAGER_H
#define MESSAGEMANAGER_H

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <vector>

#include "Client.h"

class ChannelManager;
class ClientManager;

using CommandHandler = std::function<void(std::shared_ptr<Client>, const std::vector<std::string>&)>;

class MessageManager {
public:
    MessageManager(ClientManager& clientManager, ChannelManager& channelManager);
    ~MessageManager() = default;

    MessageManager(const MessageManager&) = delete;
    MessageManager& operator=(const MessageManager&) = delete;

    void handleMessage(std::shared_ptr<Client> sender, const std::string& message);
    void handleCommand(std::shared_ptr<Client> sender, const std::string& command);

    void broadcastMessage(std::shared_ptr<Client> sender, const std::string& message);
    void sendPrivateMessage(std::shared_ptr<Client> sender, const std::string& recipient, const std::string& message);
    void sendChannelMessage(std::shared_ptr<Client> sender, const std::string& channelName, const std::string& message);
    void sendServerMessage(std::shared_ptr<Client> client, const std::string& message);

    void registerCommand(const std::string& name, CommandHandler handler);
    void unregisterCommand(const std::string& name);

    size_t getProcessedMessagesCount() const;
    size_t getProcessedCommandsCount() const;
    size_t getSentMessagesCount() const;
    size_t getReceivedBytesCount() const;
    size_t getSentBytesCount() const;

    void setMotd(const std::string& motd);
    std::string getMotd() const;

private:
    ClientManager& clientManager_;
    ChannelManager& channelManager_;
    
    std::unordered_map<std::string, CommandHandler> commandHandlers_;
    std::string motd_;

    std::atomic<size_t> processedMessages_{0};
    std::atomic<size_t> processedCommands_{0};
    std::atomic<size_t> sentMessages_{0};
    std::atomic<size_t> receivedBytes_{0};
    std::atomic<size_t> sentBytes_{0};

    std::vector<std::string> parseCommandArgs(const std::string& commandLine);
    void setupDefaultCommands();

    void handleNickCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args);
    void handleJoinCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args);
    void handlePartCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args);
    void handleQuitCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args);
    void handleListCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args);
    void handleWhoCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args);
    void handlePrivmsgCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args);
    void handleMotdCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args);
    void handleHelpCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args);
};

#endif //MESSAGEMANAGER_H