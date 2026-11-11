#include"include/Connection.h"
#include"include/Channel.h"
#include"include/GetTime.h"
#include"include/EventLoop.h"

Connection::Connection(const int& fd, EventLoop* loop) :
    read_buff_(std::make_unique<Buffer>()),
    write_buff_(std::make_unique<Buffer>()),
    channel_(std::make_unique<Channel>(fd, loop)) {
}

const std::string Connection::RecvMsg() {
    while (1) {
        while (read_buff_->readableBytes() >= 4) {
            uint32_t MSGSIZE;
            memcpy(&MSGSIZE, read_buff_->peek(), 4);
            MSGSIZE = ntohl(MSGSIZE);

            if (MSGSIZE == 0 || MSGSIZE > 4096) {
                read_buff_->hread(4);
                return "";
            }
            if (MSGSIZE + 4 > read_buff_->readableBytes()) {
                break;
            }

            read_buff_->hread(4);

            rmsg.assign(read_buff_->peek(), MSGSIZE);
            read_buff_->hread(MSGSIZE);

            return rmsg;
        }

        ssize_t n = read_buff_->readn(channel_->fd());
        if (n > 0) {
            continue;
        }

        if (n == 0) {
            std::cout << "Client " << channel_->fd() << " has disconnected..." << std::endl;
            this->DeleteConnection();
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

void Connection::SendMsg(std::string_view Msg) {
    uint32_t MSGSIZE = Msg.size();
    MSGSIZE = htonl(MSGSIZE);
    write_buff_->Append((char*)&MSGSIZE, 4);
    write_buff_->Append(Msg.data(), Msg.size());

    if (write_buff_->writen(channel_->fd()) == -1) {
        perror("writen");
    }
}



void Connection::DeleteConnection() {
    channel_->loop()->DeleteConnection(channel_->fd());
}

Channel* Connection::channel() const {
    return channel_.get();
}
