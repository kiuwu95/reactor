#include"include/ClientBuffer.h"

namespace {
    constexpr size_t MAXSIZE = 64 * 1024;
}

void Buffer::hread(const size_t& len) {
    reader_ += len;
    if (reader_ == writer_) {
        reader_ = writer_ = 0;
    }
}
void Buffer::Writable(const size_t& len) {
    if (len > writableBytes()) {
        Make(len);
    }
}
void Buffer::Append(const char* p, const size_t& len) {
    Writable(len);
    memcpy(buf_.data() + writer_, p, len);
    writer_ += len;
}

void Buffer::Make(const size_t& len) {
    size_t nread = readableBytes();
    if (readableBytes() + len <= buf_.size()) {
        if (nread != 0) {
            memmove(buf_.data(), buf_.data() + reader_, nread);
        }
        reader_ = 0;
        writer_ = nread;
        return;
    }

    size_t want = std::max(nread + len, buf_.size() * 2);
    want = std::min(want, std::max(MAXSIZE, nread + len));

    std::vector<char> temp(want);
    if (nread != 0) {
        memcpy(temp.data(), buf_.data() + reader_, readableBytes());
    }
    buf_.swap(temp);

    reader_ = 0;
    writer_ = nread;
}

ssize_t Buffer::readn(int fd) {
    if (writableBytes() == 0) {
        if (readableBytes() == 0) {
            return -1;
        }
        Make(std::max<size_t>(buf_.size() / 2, 1));
    }

    ssize_t n = recv(fd, buf_.data() + writer_, writableBytes(), 0);
    if (n >= 0) {
        writer_ += n;
        return n;
    }
    return -1;
}

ssize_t Buffer::writen(int fd) {
    uint32_t MSGSIZE = 0;
    memcpy(&MSGSIZE, this->peek(), 4);

    size_t len = ntohl(MSGSIZE);
    if (this->readableBytes() < len + 4) {
        this->hread(this->readableBytes());
        return -1;
    }

    ssize_t n;
    if ((n = send(fd, this->peek(), 4 + len, 0)) == -1) {
        perror("send");
    }
    this->hread(4 + len);

    return n;
}