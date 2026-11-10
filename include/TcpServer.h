#pragma once
#include<thread>
#include<vector>
#include<unordered_map>

class Acceptor;
class EventLoop;

class TcpServer {
public:
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer(const char IP[] = "127.0.0.1", const uint16_t& port = 9999, const size_t& loops_num = 4);
    ~TcpServer();

    void start();
private:
    void AcceptorConnectionCallBack(int fd);
private:
    std::unique_ptr<EventLoop>main_loop;
    std::unique_ptr<Acceptor>acceptor_;
    std::vector<std::unique_ptr<EventLoop>>sub_loops;
    std::vector<std::thread>threads_;
    size_t loops_index;
};