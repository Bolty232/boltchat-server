#include <iostream>
#include <string>
#include <memory>   // Für std::unique_ptr
#include <vector>   // Nützlich für die Argumentenverarbeitung
#include <csignal>  // Für die Signalbehandlung (Strg+C)

#include "Server.h"

std::unique_ptr<Server> g_server;

void signalHandler(int signum) {
    std::cout << "\nShutdown signal (" << signum << ") received." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options]\n"
              << "Options:\n"
              << "  -h, --help            Show this help message\n"
              << "  -cp, --configpath     <path> Path to the configuration file\n"
              //<< "  -p, --port            <port> Override the port from the config file\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::string configPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "-cp" || arg == "--configpath") {
            if (i + 1 < argc) {
                configPath = argv[++i];
            } else {
                std::cerr << "Error: --configpath option requires a value." << std::endl;
                return 1;
            }
        }
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    try {
        if (!configPath.empty()) {
            std::cout << "Loading configuration from: " << configPath << std::endl;
            g_server = std::make_unique<Server>(configPath);
        } else {
            std::cout << "No configuration file provided, using default debug configuration." << std::endl;
            g_server = std::make_unique<Server>();
        }

        g_server->start();

    } catch (const std::exception& e) {
        std::cerr << "A fatal error occurred during server startup or execution: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server shut down gracefully." << std::endl;
    return 0;
}