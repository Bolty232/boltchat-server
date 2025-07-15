cmake_minimum_required(VERSION 3.31)

project(Server)
set(CMAKE_CXX_STANDARD 20)
add_executable(Server src/main.cpp src/Server.cpp include/Server.h
        include/ConfigReader.h
        src/ConfigReader.cpp
        include/ThreadPool.h
        src/ThreadPool.cpp
        src/Client.cpp
        include/Client.h
        src/Channel.cpp
        include/Channel.h
        include/ChannelManager.h
        src/ChannelManager.cpp
        include/ClientManager.h
        src/ClientManager.cpp
        include/MessageManager.h
        src/MessageManager.cpp
)
target_include_directories(Server PRIVATE include)