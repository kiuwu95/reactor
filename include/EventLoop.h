#pragma once
#include<sys/epoll.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<memory>
#include<atomic>
#include<functional>
#include<unordered_map>

#include"MemoryPool.h"
class Channel;
class Connection;

class EventLoop {
public:
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(const EventLoop&) = delete;

    EventLoop();
    ~EventLoop();

    void AddChannel(Channel* Ch);
    void AddConnection(std::unique_ptr<Connection> Conn);
    void DeleteConnection(int fd);
    void RunInLoop(std::function<void()>f);

    void loop();

    int epfd() const;
    static MemoryPool& GetMemoryPool() { return mp_; }
private:
    int epfd_;
    std::atomic<bool> end_;
    std::unordered_map<int, std::unique_ptr<Connection>> Connections_;
    std::vector<int> connectionsBin;

    static MemoryPool mp_;
};