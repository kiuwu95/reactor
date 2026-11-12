#include"include/EventLoop.h"
#include"include/Channel.h"
#include"include/Connection.h"

EventLoop::EventLoop() :epfd_(epoll_create1(0)),
ev_channel_(std::make_unique<Channel>(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK), this)) {
    if (epfd_ == -1) {
        perror("epoll_create1");
    }

    ev_channel_->SetReadCallBack([this] {
        uint64_t zero;
        ssize_t n = read(ev_channel_->fd(), &zero, sizeof(zero));
        (void)n;
        });
    ev_channel_->EnableReading();
}

EventLoop::~EventLoop() {
    end_ = true;
    EnableEventFd();//使epoll_wait不再阻塞
}
//将fd添加至epoll
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

void EventLoop::RunInLoop(std::function<void()> f) {
    if (std::this_thread::get_id() == thread_id_) {
        f();
    }
    else {
        tasks_.emplace_back(std::move(f));
    }
}

void EventLoop::loop() {
    thread_id_ = std::this_thread::get_id();

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

        HandleTasks();
    }
}

void EventLoop::HandleTasks() {
    std::vector<std::function<void()>>tasks;
    {
        std::unique_lock<std::mutex> lock(mtx_);
        tasks.swap(tasks_);
    }
    for (auto& it : tasks) {
        it();
    }
}

void EventLoop::EnableEventFd() {
    uint64_t one = 1;
    ssize_t n = write(ev_channel_->fd(), &one, sizeof(one));
    (void)n;
}

void EventLoop::AddTask(std::function<void()> f) {
    {
        std::unique_lock<std::mutex> lock(mtx_);
        tasks_.emplace_back(std::move(f));
    }
    EnableEventFd();
}
