#include"include/EventLoop.h"
#include"include/Channel.h"
#include"include/Connection.h"

EventLoop::EventLoop() :epfd_(epoll_create1(0)), end_(false) {
    if (epfd_ == -1) {
        perror("epoll_create1");
    }
}

EventLoop::~EventLoop() {
    end_ = true;
}

void EventLoop::AddChannel(Channel* Ch) {
    epoll_event ev;
    ev.data.ptr = Ch;
    ev.events = Ch->events();
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, Ch->fd(), &ev) == -1) {
        perror("epoll_ctl");
    }
}

void EventLoop::AddConnection(std::unique_ptr<Connection> conn) {
    Connections_.emplace(conn->channel()->fd(), std::move(conn));
}

void EventLoop::DeleteConnection(int fd) {
    connectionsBin.emplace_back(fd);
}

void EventLoop::RunInLoop(std::function<void()>f) { f(); }

void EventLoop::loop() {
    epoll_event events_[1024];
    while (!end_) {
        int num = epoll_wait(epfd_, events_, 1024, -1);
        for (int i = 0;i < num;i++) {
            Channel* ch = static_cast<Channel*>(events_[i].data.ptr);
            ch->HandleEvent();
        }

        for (auto& it : connectionsBin) {
            Connections_.erase(it);
        }
        connectionsBin.clear();

    }
}

int EventLoop::epfd() const { return epfd_; }