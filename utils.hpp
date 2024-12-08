#pragma once

#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <string>
#include <ctime>
#include <map>
#include <functional>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <errno.h>
#include <thread>
#include <algorithm>
#include <unordered_set>
#include <sys/inotify.h>
#include <signal.h>

class Utils{
private:
    const std::string LogPath = "/sdcard/Android/MW_CpuSpeedController/log.txt";
public:
    void clear_log() {
        std::ofstream ofs;
        ofs.open(LogPath, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    }

    void log(const std::string& message) {
        std::time_t now = std::time(nullptr);
        std::tm* local_time = std::localtime(&now);
        char time_str[100];
        std::strftime(time_str, sizeof(time_str), "[%Y-%m-%d %H:%M:%S]", local_time);

        std::ofstream logfile(LogPath, std::ios_base::app);
        if (logfile.is_open()) {
            logfile << time_str << " " << message << std::endl;
            logfile.close();
        }
    }
    void WriteFile(const std::string& filePath, const std::string& content) noexcept {
        int fd = open(filePath.c_str(), O_WRONLY | O_NONBLOCK);

        if (fd < 0) {
            chmod(filePath.c_str(), 0666);
            fd = open(filePath.c_str(), O_WRONLY | O_NONBLOCK);
        }

        if (fd >= 0) {
            write(fd, content.data(), content.size());
            close(fd);
            chmod(filePath.c_str(), 0444);
        }
    }
    std::unordered_set<std::string> getTopAppPids() {
        std::unordered_set<std::string> topAppPids;
        std::ifstream cgroupFile("/dev/cpuset/top-app/cgroup.procs");

        if (!cgroupFile.is_open()) {
            return topAppPids; // 返回空集合
        }

        std::string pid;
        while (std::getline(cgroupFile, pid)) {
            if (!pid.empty()) {
                topAppPids.insert(pid);
            }
        }
        return topAppPids;
    }



};