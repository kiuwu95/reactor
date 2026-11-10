#pragma once
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdio.h>
#include<string> 
#include<iostream>
#include<string_view>
#include<thread>
#include<atomic>
#include<memory>

class Buffer;

class Client {
public:
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    Client();
    ~Client();

    bool ConnectServer(const char IP[] = "127.0.0.1", const uint16_t& port = 9999);
    void StartListen();

    void GetMessage();
    void SendMsg(std::string_view Msg);
    std::string RecvMsg();
private:
    int cfd_;
    std::atomic<bool> stopListen_ = true, end_ = false;
    std::string rmsg;

    std::thread ListenThread_;

    std::unique_ptr<Buffer> read_buff_;
    std::unique_ptr<Buffer> write_buff_;
};