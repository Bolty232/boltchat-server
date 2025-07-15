#include "MessageManager.h"
#include "ClientManager.h"
#include "ChannelManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>

MessageManager::MessageManager(ClientManager& clientManager, ChannelManager& channelManager)
    : clientManager_(clientManager), channelManager_(channelManager) {
    setupDefaultCommands();
}

void MessageManager::handleMessage(std::shared_ptr<Client> sender, const std::string& message) {
    if (!sender) return;
    processedMessages_++;
    receivedBytes_ += message.length();
    std::string cleanMessage = message;
    if (!cleanMessage.empty() && cleanMessage.back() == '\r') {
        cleanMessage.pop_back();
    }
    if (cleanMessage.empty()) return;

    if (cleanMessage.starts_with("/")) {
        handleCommand(sender, cleanMessage);
    } else {
        const std::string& activeChannel = sender->getActiveChannel();
        if (!activeChannel.empty()) {
            sendChannelMessage(sender, activeChannel, cleanMessage);
        } else {
            sendServerMessage(sender, "You are not in any channel. Join one with /join <#channel> or send a private message with /msg <user> <message>.");
        }
    }
}

void MessageManager::handleCommand(std::shared_ptr<Client> sender, const std::string& commandLine) {
    if (!sender || commandLine.empty()) return;
    processedCommands_++;
    auto args = parseCommandArgs(commandLine);
    if (args.empty()) return;
    std::string command = args[0].substr(1);
    args.erase(args.begin());
    auto it = commandHandlers_.find(command);
    if (it != commandHandlers_.end()) {
        it->second(sender, args);
    } else {
        sendServerMessage(sender, "Unknown command: " + command);
    }
}

void MessageManager::broadcastMessage(std::shared_ptr<Client> sender, const std::string& message) {
    if (!sender) return;
    sentMessages_++;
    sentBytes_ += message.length();
    clientManager_.broadcastMessage(message, sender);
}

void MessageManager::sendPrivateMessage(std::shared_ptr<Client> sender, const std::string& recipient, const std::string& message) {
    if (!sender) return;
    auto targetClient = clientManager_.getClientByNickname(recipient);
    if (!targetClient) {
        sendServerMessage(sender, "User " + recipient + " not found.");
        return;
    }
    sentMessages_++;
    sentBytes_ += message.length();
    std::string formattedMsg = "*Private from " + sender->getNickname() + ": " + message;
    clientManager_.sendMessageToClient(targetClient, formattedMsg);
    std::string copyMsg = "*Private to " + recipient + ": " + message;
    clientManager_.sendMessageToClient(sender, copyMsg);
}

void MessageManager::sendChannelMessage(std::shared_ptr<Client> sender, const std::string& channelName, const std::string& message) {
    if (!sender) return;
    if (!channelManager_.channelExists(channelName)) {
        sendServerMessage(sender, "Channel " + channelName + " does not exist.");
        return;
    }
    auto clientChannels = channelManager_.getClientChannels(sender);
    bool isInChannel = std::find(clientChannels.begin(), clientChannels.end(), channelName) != clientChannels.end();
    if (!isInChannel) {
        sendServerMessage(sender, "You are not in channel " + channelName);
        return;
    }
    sentMessages_++;
    sentBytes_ += message.length();
    std::string formattedMsg = "<" + sender->getNickname() + "@" + channelName + "> " + message;
    channelManager_.broadcastToChannel(channelName, formattedMsg);
}

void MessageManager::sendServerMessage(std::shared_ptr<Client> client, const std::string& message) {
    if (!client) return;
    sentMessages_++;
    sentBytes_ += message.length();
    std::string formattedMsg = "*** " + message;
    clientManager_.sendMessageToClient(client, formattedMsg);
}

void MessageManager::registerCommand(const std::string& name, CommandHandler handler) {
    commandHandlers_[name] = std::move(handler);
}

void MessageManager::unregisterCommand(const std::string& name) {
    commandHandlers_.erase(name);
}

size_t MessageManager::getProcessedMessagesCount() const { return processedMessages_.load(); }
size_t MessageManager::getProcessedCommandsCount() const { return processedCommands_.load(); }
size_t MessageManager::getSentMessagesCount() const { return sentMessages_.load(); }
size_t MessageManager::getReceivedBytesCount() const { return receivedBytes_.load(); }
size_t MessageManager::getSentBytesCount() const { return sentBytes_.load(); }
void MessageManager::setMotd(const std::string& motd) { motd_ = motd; }
std::string MessageManager::getMotd() const { return motd_; }

std::vector<std::string> MessageManager::parseCommandArgs(const std::string& commandLine) {
    std::vector<std::string> args;
    std::istringstream iss(commandLine);
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }
    return args;
}

void MessageManager::setupDefaultCommands() {
    registerCommand("nick", [this](auto c, auto a){ this->handleNickCommand(c, a); });
    registerCommand("join", [this](auto c, auto a){ this->handleJoinCommand(c, a); });
    registerCommand("part", [this](auto c, auto a){ this->handlePartCommand(c, a); });
    registerCommand("quit", [this](auto c, auto a){ this->handleQuitCommand(c, a); });
    registerCommand("list", [this](auto c, auto a){ this->handleListCommand(c, a); });
    registerCommand("who", [this](auto c, auto a){ this->handleWhoCommand(c, a); });
    registerCommand("msg", [this](auto c, auto a){ this->handlePrivmsgCommand(c, a); });
    registerCommand("motd", [this](auto c, auto a){ this->handleMotdCommand(c, a); });
    registerCommand("help", [this](auto c, auto a){ this->handleHelpCommand(c, a); });
}

void MessageManager::handleNickCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args) {
    if (args.empty()) {
        sendServerMessage(client, "Usage: /nick <new_nick>");
        return;
    }
    std::string newNickname = args[0];
    if (clientManager_.clientExistsByNickname(newNickname) && clientManager_.getClientByNickname(newNickname) != client) {
        sendServerMessage(client, "Nickname '" + newNickname + "' already in use.");
        return;
    }
    std::string oldNickname = client->getNickname();
    if (!clientManager_.updateClientNickname(client, newNickname)) {
        sendServerMessage(client, "Nickname '" + newNickname + "' is not valid or already in use.");
        return;
    }
    sendServerMessage(client, "Nickname switched to '" + newNickname + "'");
    std::string notificationMsg = "User '" + oldNickname + "' is now known as '" + newNickname + "'";
    clientManager_.broadcastMessage(notificationMsg);
}

void MessageManager::handleJoinCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args) {
    if (args.empty()) {
        sendServerMessage(client, "Usage: /join <#channel>");
        return;
    }
    std::string channelName = args[0];
    if (!channelName.starts_with("#")) {
        channelName = "#" + channelName;
    }
    if (channelManager_.joinChannel(client, channelName)) {
        client->setActiveChannel(channelName);
        sendServerMessage(client, "You joined " + channelName + " (now active).");
        std::string joinMsg = client->getNickname() + " joined the channel.";
        channelManager_.broadcastToChannel(channelName, "*** " + joinMsg);
    } else {
        sendServerMessage(client, "Could not join " + channelName);
    }
}

void MessageManager::handlePartCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args) {
    if (args.empty()) {
        sendServerMessage(client, "Usage: /part <#channel>");
        return;
    }
    std::string channelName = args[0];
    if (!channelName.starts_with("#")) {
        channelName = "#" + channelName;
    }
    auto clientChannels = channelManager_.getClientChannels(client);
    bool isInChannel = std::find(clientChannels.begin(), clientChannels.end(), channelName) != clientChannels.end();
    if (!isInChannel) {
        sendServerMessage(client, "You are not in channel " + channelName);
        return;
    }
    std::string partMsg = client->getNickname() + " left the channel.";
    channelManager_.broadcastToChannel(channelName, "*** " + partMsg);
    if (channelManager_.leaveChannel(client, channelName)) {
        sendServerMessage(client, "You have left " + channelName);
    } else {
        sendServerMessage(client, "Error leaving channel " + channelName);
    }
}

void MessageManager::handleQuitCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args) {
    std::string quitMessage = "Client quit.";
    if (!args.empty()) {
        quitMessage = args[0];
        for (size_t i = 1; i < args.size(); ++i) {
            quitMessage += " " + args[i];
        }
    }
    auto channels = channelManager_.getClientChannels(client);
    std::string notificationMsg = client->getNickname() + " left the server: " + quitMessage;
    for (const auto& channelName : channels) {
        channelManager_.broadcastToChannel(channelName, "*** " + notificationMsg);
    }
    clientManager_.removeClient(client);
}

void MessageManager::handleListCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args) {
    auto channels = channelManager_.getChannelList();
    if (channels.empty()) {
        sendServerMessage(client, "No active channels.");
        return;
    }
    sendServerMessage(client, "Active channels:");
    for (const auto& channelName : channels) {
        size_t memberCount = channelManager_.getChannelMemberCount(channelName);
        sendServerMessage(client, "- " + channelName + " (" + std::to_string(memberCount) + " members)");
    }
}

void MessageManager::handleWhoCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args) {
    if (args.empty()) {
        auto clients = clientManager_.getAllClients();
        if (clients.empty()) {
            sendServerMessage(client, "No users online.");
            return;
        }
        sendServerMessage(client, "Online users (" + std::to_string(clients.size()) + "):");
        for (const auto& c : clients) {
            auto channels = channelManager_.getClientChannels(c);
            std::string channelsStr;
            if (!channels.empty()) {
                channelsStr = " in: ";
                for (size_t i = 0; i < channels.size(); ++i) {
                    channelsStr += channels[i] + (i < channels.size() - 1 ? ", " : "");
                }
            }
            sendServerMessage(client, "- " + c->getNickname() + channelsStr);
        }
    } else {
        std::string channelName = args[0];
        if (!channelName.starts_with("#")) {
            channelName = "#" + channelName;
        }
        auto channelOpt = channelManager_.getChannel(channelName);
        if(channelOpt) {
            auto nicknames = channelOpt.value().get().getMemberNicknames();
            sendServerMessage(client, "Users in " + channelName + " (" + std::to_string(nicknames.size()) + "):");
            for(const auto& nick : nicknames) {
                sendServerMessage(client, "- " + nick);
            }
        } else {
            sendServerMessage(client, "Channel " + channelName + " does not exist.");
        }
    }
}

void MessageManager::handlePrivmsgCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        sendServerMessage(client, "Usage: /msg <#channel_or_user> <message>");
        return;
    }
    std::string recipient = args[0];
    std::string message;
    for (size_t i = 1; i < args.size(); ++i) {
        message += args[i] + (i < args.size() - 1 ? " " : "");
    }
    if (recipient.starts_with("#")) {
        sendChannelMessage(client, recipient, message);
    } else {
        sendPrivateMessage(client, recipient, message);
    }
}

void MessageManager::handleMotdCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args) {
    if (motd_.empty()) {
        sendServerMessage(client, "No MOTD available.");
    } else {
        sendServerMessage(client, "Message of the Day:");
        sendServerMessage(client, motd_);
    }
}

void MessageManager::handleHelpCommand(std::shared_ptr<Client> client, const std::vector<std::string>& args) {
    sendServerMessage(client, "Available commands:");
    sendServerMessage(client, "/nick <name>              - Change your nickname");
    sendServerMessage(client, "/join <#channel>          - Join a channel");
    sendServerMessage(client, "/part <#channel>          - Leave a channel");
    sendServerMessage(client, "/msg <#channel|user> <msg> - Send a message to a channel or user");
    sendServerMessage(client, "/list                     - List all active channels");
    sendServerMessage(client, "/who [#channel]           - List users on server or in a channel");
    sendServerMessage(client, "/motd                     - Show the Message of the Day");
    sendServerMessage(client, "/quit [message]           - Disconnect from the server");
    sendServerMessage(client, "/help                     - Show this help message");
}