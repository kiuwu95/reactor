#pragma once

#include<string.h>
#include<vector>
#include<stdio.h>
#include<arpa/inet.h>

class Buffer {
public:
    Buffer(const size_t& init = 4096) :buf_(init) {}

    size_t readableBytes() const { return writer_ - reader_; }
    size_t writableBytes() const { return buf_.size() - writer_; }
    const char* peek() const { return buf_.data() + reader_; }

    void hread(const size_t& len);
    void Writable(const size_t& len);
    void Append(const char* p, const size_t& len);
    void Make(const size_t& len);

    ssize_t readn(int fd);
    ssize_t writen(int fd);
private:
    std::vector<char> buf_;
    size_t reader_ = 0;
    size_t writer_ = 0;
};