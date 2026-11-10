#pragma once
#include<stdio.h>
#include<errno.h>
#include<cstdint>
#include<memory>
#include<iostream>
#include<functional>
#include<fcntl.h>

class EventLoop;
class Channel;

class Acceptor {
public:
    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    Acceptor(EventLoop* loop, const uint16_t& port, const char IP[]);

    void SetConnectionCallBack(std::function<void(int)>cb);
private:
    void AcceptClient();
private:
    EventLoop* main_loop;
    std::unique_ptr<Channel> listen_channel;
    std::function<void(int)>conn_cb;
};