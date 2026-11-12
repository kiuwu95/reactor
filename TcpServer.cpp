#include"include/TcpServer.h"
#include"include/EventLoop.h"
#include"include/Channel.h"
#include"include/Acceptor.h"
#include"include/Connection.h"
#include"include/GetTime.h"


TcpServer::TcpServer(const char IP[], const uint16_t& port, const size_t& loops_num) :
    main_loop(std::make_unique<EventLoop>()),
    acceptor_(std::make_unique<Acceptor>(main_loop.get(), port, IP)),
    loops_index(0) {
    acceptor_->SetConnectionCallBack([this](int fd) {AcceptorConnectionCallBack(fd);});

    for (size_t i = 0;i < loops_num;i++) {
        std::unique_ptr<EventLoop>sub_loop = std::make_unique<EventLoop>();
        sub_loops.emplace_back(std::move(sub_loop));
    }
}
TcpServer::~TcpServer() {
    for (auto& it : threads_) {
        it.join();
    }
}

void TcpServer::start() {
    for (auto& it : sub_loops) {
        threads_.emplace_back([ptr = it.get()] {
            ptr->loop();
            });
    }
    threads_.emplace_back([this] {
        main_loop->loop();
        });
}

void TcpServer::AcceptorConnectionCallBack(int fd) {
    EventLoop* loop = sub_loops[loops_index++ % sub_loops.size()].get();

    loop->RunInLoop([fd, loop, this] {
        std::unique_ptr<Connection> conn = std::make_unique<Connection>(fd, loop);
        Connection* ptr = conn.get();

        conn->channel()->SetReadCallBack([fd, ptr] {
            std::string msg;
            while (!(msg = ptr->RecvMsg()).empty() && msg != "") {
                std::cout << GetTime::GetCurrentTime()
                    << "Client " << fd << ": " << msg << std::endl;
            }
            });

        loop->AddConnection(std::move(conn));
        ptr->channel()->EnableReading();
        });

}
