#pragma once
#include<sys/epoll.h>
#include<sys/eventfd.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<memory>
#include<atomic>
#include<mutex>
#include<thread>
#include<functional>
#include<unordered_map>

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
    void RunInLoop(std::function<void()> f);

    void loop();

    int epfd() const { return epfd_; };

    void HandleTasks();
    void EnableEventFd();
    void AddTask(std::function<void()> f);
private:
    int epfd_;
    std::atomic<bool> end_ = false;

    std::unordered_map<int, std::unique_ptr<Connection>> Connections_;
    std::vector<int> connectionsBin;

    std::unique_ptr<Channel> ev_channel_;
    std::vector <std::function<void()>>tasks_;
    std::mutex mtx_;
    std::thread::id thread_id_;
};