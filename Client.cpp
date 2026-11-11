#include"include/Client.h"
#include"include/GetTime.h"
#include"include/Buffer.h"

Client::Client() :cfd_(socket(AF_INET, SOCK_STREAM, 0)),
read_buff_(std::make_unique<Buffer>()),
write_buff_(std::make_unique<Buffer>()) {
    if (cfd_ == -1) {
        perror("socket");
    }
}
Client::~Client() {
    end_ = stopListen_ = true;
    ListenThread_.join();

    if (cfd_ > 0) {
        close(cfd_);
    }
}

bool Client::ConnectServer(const char IP[], const uint16_t& port) {
    sockaddr_in saddr;
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    inet_pton(AF_INET, IP, &saddr.sin_addr.s_addr);

    if (connect(cfd_, (sockaddr*)&saddr, sizeof(saddr)) == -1) {
        perror("connect");
        return 0;
    }

    stopListen_ = false;
    this->StartListen();

    std::cout << GetTime::GetCurrentTime();
    std::cout << "Connected to the server successfully!" << std::endl;

    return 1;
}

void Client::StartListen() {
    ListenThread_ = std::thread([this] {
        std::string recv_msg;
        while (!end_) {
            while (!stopListen_) {
                if (!(recv_msg = this->RecvMsg()).size() && recv_msg != "") {
                    std::cout << GetTime::GetCurrentTime() << recv_msg << std::endl;
                }
            }
        }
        });
}

void Client::GetMessage() {
    std::string msg;
    while (1) {
        std::getline(std::cin, msg);
        this->SendMsg(msg);
    }
}

void Client::SendMsg(std::string_view Msg) {
    uint32_t MSGSIZE = Msg.size();
    MSGSIZE = htonl(MSGSIZE);
    write_buff_->Append((char*)&MSGSIZE, 4);
    write_buff_->Append(Msg.data(), Msg.size());

    if (write_buff_->writen(cfd_) == -1) {
        perror("writen");
    }
}

std::string Client::RecvMsg() {
    while (1) {
        while (read_buff_->readableBytes() >= 4) {
            uint32_t MSGSIZE;
            memcpy(&MSGSIZE, read_buff_->peek(), 4);
            MSGSIZE = ntohl(MSGSIZE);

            if (MSGSIZE == 0) {
                read_buff_->hread(4);
                return "";
            }
            if (MSGSIZE + 4 > read_buff_->readableBytes()) {
                break;
            }

            read_buff_->hread(4);

            memcpy(rmsg.data(), read_buff_->peek(), MSGSIZE);
            read_buff_->hread(MSGSIZE);

            return rmsg;
        }

        ssize_t n = read_buff_->readn(cfd_);
        if (n > 0) {
            continue;
        }

        if (n == 0) {
            std::cout << "Server has disconnected..." << std::endl;
            stopListen_ = true;
            return "";
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return "";
        }
        if (errno == EINTR) {
            continue;
        }
        perror("recv");
        return "";
    }
}