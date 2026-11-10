#pragma once
#include<arpa/inet.h>
#include<string.h>
#include<errno.h>
#include<string>
#include<iostream>
#include<memory>

#include"ServerBuffer.h"
class Channel;
class EventLoop;

class Connection {
public:
    Connection& operator=(const Connection&) = delete;
    Connection(const Connection&) = delete;

    Connection() = delete;
    Connection(const int& fd, EventLoop* loop);
    ~Connection() = default;

    const std::string RecvMsg();
    void SendMsg(std::string_view Msg);

    void DeleteConnection();

    Channel* channel() const;

    static void* operator new(size_t size) {
        return EventLoop::GetMemoryPool().allocate();
    }
    static void operator delete(void* p) noexcept {
        return EventLoop::GetMemoryPool().deallocate(p);
    }
private:
    std::unique_ptr<Buffer>read_buff_;
    std::unique_ptr<Buffer>write_buff_;
    std::unique_ptr<Channel> channel_;

    std::string rmsg;
};