#pragma once

#include "utils.hpp"
#include "INIreader.hpp"

class A0_Memory {
private:
    bool Kill_Process;
    bool Standby_Process;
    bool Disable_mglru;
    bool Zip_Process;
    bool Hook_Lmkd;
    bool A0_Auto_memory;
    Utils utils;
    INIReader reader;
    std::atomic<int> RefreshTopApp{0};
    std::unordered_set<std::string> Kill_Process_list;
    const std::string conf_path = "/sdcard/Android/MW_A0/配置文件.ini";
    const std::string kill_list = "/sdcard/Android/MW_A0/乖巧列表.conf";
    const std::string White_List = "/sdcard/Android/MW_A0/白名单.conf";
    const std::string Background_Process = "/dev/cpuset/Background/cgroup.procs";
    const std::string Mglru_Path = "/sys/kernel/mm/lru_gen/enabled";
    const std::string Proc_path = "/proc/";
public:
    A0_Memory() : reader("/sdcard/Android/MW_CpuSpeedController/config.ini") {}
    void readAndParseConfig() {
        INIReader
                reader("/sdcard/Android/MW_A0/配置文件.ini");

        if (reader.ParseError() < 0) {
            utils.log("警告:请检查您的配置文件是否存在");
            exit(0);
        }

         Kill_Process = reader.GetBoolean("meta", "乖巧模式", false);
         Standby_Process = reader.GetBoolean("meta", "休眠进程", false);
         Disable_mglru = reader.GetBoolean("meta", "关闭Mglru", false);
         Zip_Process = reader.GetBoolean("meta", "压缩内存", false);
         Hook_Lmkd = reader.GetBoolean("meta", "HookLmkd", false);
         A0_Auto_memory = reader.GetBoolean("meta", "A0内存管理", false);
    }
    void Error_Handle(){
        utils.log("错误:请检查乖巧模式配置文件或白名单文件是否存在,将在两秒后结束进程");
        sleep(2);
        exit(0);
    }
    void get_KillProcesss_List() {
        if (!check_WhiteList_Path())
            Error_Handle();

        std::ifstream file(kill_list);

        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                Kill_Process_list.insert(line);
            }
        }
    }


    void getWhite_List(){
        if (!check_WhiteList_Path())
            Error_Handle();
    }

    void kill_Process() {
        if (!Kill_Process)
            return;

        // 当应用切换次数到3时 开始执行下面的代码 动态更新还可能需要多Fork几个线程去监听配置文件的变化
        get_KillProcesss_List();
        if (RefreshTopApp == 3) {

            RefreshTopApp.store(0, std::memory_order_relaxed); // 值清零
        }
    }

    void Disable_Mglru(){
        if(!Disable_mglru)
            return;

        if (checkMglru_Path()) {
            utils.WriteFile("/sys/kernel/mm/lru_gen/enabled", "N");
            utils.log("已关闭Mglru");
        }
    }
    void compress_Process(){
        if(!Zip_Process)
            return;
        /*
         * 先读取后台pid然后存储在字符串中 然后使用WriteFile函数写入到/proc/pid/
         */
    }
    void A0_Auto_Memory(){
        if(!A0_Auto_memory)
            return;

    }
    void standby_Process() {
        if(!Standby_Process)
            return;
        /*
         * 这里没必要使用原子
         */
        while(1){
            // 这里使用exec管道进入standby模式 并将白名单中的应用加入到白名单


            sleep (120);
        }
    }

    bool check_KillProcess_Path(){
        return access(kill_list.c_str(), F_OK) == 0;
    }

    bool check_WhiteList_Path(){
        return access(White_List.c_str(), F_OK) == 0;
    }

    bool checkMglru_Path(){
        return access(Mglru_Path.c_str(), F_OK) == 0;
    }

    void ReFreshTopApp(){
        std::thread getReFreshTopApp(&A0_Memory::InotifyMain, this, "/dev/cpuset/top-app/cgroup.procs", IN_MODIFY);
        getReFreshTopApp.detach();
    }

    int InotifyMain(const char* dir_name, uint32_t mask) {
        int fd = inotify_init();
        if (fd < 0) {
            printf("Failed to initialize inotify. \n");
            return -1;
        }

        int wd = inotify_add_watch(fd, dir_name, mask);
        if (wd < 0) {
            printf("Failed to add watch for directory: %s \n", dir_name);
            close(fd);
            return -1;
        }

        const int buflen = sizeof(struct inotify_event) + NAME_MAX + 1;
        char buf[buflen];
        fd_set readfds;

        while (true) {
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);

            int iRet = select(fd + 1, &readfds, nullptr, nullptr, nullptr);
            if (iRet < 0) {
                break;
            }

            int len = read(fd, buf, buflen);
            if (len < 0) {
                printf("Failed to read inotify events. \n");
                break;
            }
            const struct inotify_event* event = reinterpret_cast<const struct inotify_event*>(buf);
            if (event->mask & mask) {
                RefreshTopApp.fetch_add(1, std::memory_order_relaxed); // 事件发生变化就+1
            }
        }

        inotify_rm_watch(fd, wd);
        close(fd);

        return 0;
    }
    std::string getPids() {
        DIR* dir = opendir("/proc");
        if (dir == nullptr) {
            utils.log("错误:无法打开/proc 目录");
            return "";
        }

        std::ostringstream Pids;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR && std::all_of(entry->d_name, entry->d_name + strlen(entry->d_name), ::isdigit)) {
                pid_t pid = static_cast<pid_t>(std::stoi(entry->d_name));
                std::string cmdlinePath = "/proc/" + std::string(entry->d_name) + "/cmdline";

                std::ifstream cmdlineFile(cmdlinePath);
                if (cmdlineFile) {
                    std::string cmdline;
                    std::getline(cmdlineFile, cmdline, '\0');
                    for (const auto& process : Kill_Process_list) {
                        if (cmdline.find(process) != std::string::npos) {
                            Pids << pid << '\n';
                            break;
                        }
                    }
                }
            }
        }
        closedir(dir);
        return Pids.str();
    }
};