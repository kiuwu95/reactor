#pragma once

#include<chrono>
#include<string>
#include<ctime>

class GetTime {
public:
    GetTime& operator=(const GetTime&) = delete;
    GetTime(const GetTime&) = delete;
    GetTime() = delete;
    ~GetTime() = delete;

    static const std::string GetCurrentTime() {
        auto current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        tm cur_time;
        localtime_r(&current_time, &cur_time);

        char time[64];
        snprintf(time, sizeof(time), "[%4d-%02d-%02d %02d:%02d:%02d]", cur_time.tm_year + 1900,
            cur_time.tm_mon + 1, cur_time.tm_mday, cur_time.tm_hour, cur_time.tm_min, cur_time.tm_sec);
        return std::string(time);
    }
};