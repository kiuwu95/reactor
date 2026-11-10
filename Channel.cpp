#include"include/Channel.h"

Channel::Channel(const int& fd, EventLoop* loop) :
    fd_(fd), loop_(loop) {
    bzero(&event, sizeof(event));

    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
};
Channel::~Channel() {
    if (epoll_ctl(loop_->epfd(), EPOLL_CTL_DEL, fd_, nullptr) == -1) {
        perror("epoll_ctl");
    }
    close(fd_);
}

int Channel::fd() const { return fd_; }
uint32_t Channel::events() const { return event; }
EventLoop* Channel::loop() const { return loop_; }

void Channel::UpdateEvent(const uint32_t& NewEvent) { event = NewEvent; }
void Channel::EnableReading() {
    event |= EPOLLIN;
    loop_->AddChannel(this);
}
void Channel::HandleEvent() {
    if (event & EPOLLIN) {
        readcall_back();
    }
    if (event & EPOLLOUT) {
        writecall_back();
    }
}

void Channel::SetReadCallBack(std::function<void()> f) {
    readcall_back = std::move(f);
}
void Channel::SetWriteCallBack(std::function<void()> f) {
    writecall_back = std::move(f);
}