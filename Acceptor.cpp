#include"include/Acceptor.h"
#include"include/EventLoop.h"
#include"include/Channel.h"
#include"include/GetTime.h"

Acceptor::Acceptor(EventLoop* loop, const uint16_t& port, const char IP[]) :main_loop(loop) {
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("socket");
        return;
    }
    fcntl(lfd, F_SETFL, fcntl(lfd, F_GETFL, 0) | O_NONBLOCK);

    sockaddr_in saddr;
    saddr.sin_port = htons(port);
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, IP, &saddr.sin_addr.s_addr);
    if (bind(lfd, (sockaddr*)&saddr, sizeof(saddr)) == -1) {
        perror("bind");
        return;
    }
    if (listen(lfd, SOMAXCONN) == -1) {
        perror("listen");
        return;
    }

    listen_channel = std::make_unique<Channel>(lfd, loop);

    listen_channel->SetReadCallBack([this] {AcceptClient();});

    listen_channel->EnableReading();
}
void Acceptor::SetConnectionCallBack(std::function<void(int)> cb) {
    conn_cb = std::move(cb);
}
void Acceptor::AcceptClient() {
    while (1) {
        sockaddr_in caddr;
        socklen_t addr_len = sizeof(caddr);
        bzero(&caddr, sizeof(caddr));
        int cfd = accept(listen_channel->fd(), (sockaddr*)&caddr, &addr_len);
        if (cfd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            perror("accept");
            break;
        }

        char IP[32];
        std::cout << GetTime::GetCurrentTime();
        std::cout << "A new client has connected!"
            << "    IP: " << inet_ntop(AF_INET, &caddr.sin_addr.s_addr, IP, sizeof(IP))
            << " Port: " << caddr.sin_port << std::endl;

        conn_cb(cfd);
    }
}