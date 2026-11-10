#pragma once
#include<cstdint>
#include<functional>
#include<sys/epoll.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>

#include"EventLoop.h"

class Channel {
public:
    Channel(const int& fd, EventLoop* loop);
    ~Channel();

    int fd() const;
    uint32_t events() const;
    EventLoop* loop() const;

    void UpdateEvent(const uint32_t& NewEvent);
    void EnableReading();
    void HandleEvent();

    void SetReadCallBack(std::function<void()> f);
    void SetWriteCallBack(std::function<void()> f);

    static void* operator new(size_t size) {
        return EventLoop::GetMemoryPool().allocate();
    }
    static void operator delete(void* p) noexcept {
        return EventLoop::GetMemoryPool().deallocate(p);
    }
private:
    int fd_;
    EventLoop* loop_;
    uint32_t event;

    std::function<void()> readcall_back;
    std::function<void()> writecall_back;
}; 