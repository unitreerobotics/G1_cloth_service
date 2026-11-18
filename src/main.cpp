#include "cloth_service.h"
#include <memory>
#include "JqDds.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "unitree/common/os.hpp"
#include "unitree/robot/channel/channel_factory.hpp"
#include "utils/bp_logger.hpp"

namespace {

// 传感服串口为 CH343 USB 转串口（cdc_acm 驱动，节点 /dev/ttyACMx）
constexpr const char* kUsbVid = "1a86";
constexpr const char* kUsbPid = "55d3";
constexpr int kBaudrate = 921600;

std::atomic<bool> g_running{true};

void handleSignal(int)
{
    g_running = false;
}

std::string readFirstLine(const std::filesystem::path& path)
{
    std::ifstream f(path);
    std::string line;
    std::getline(f, line);
    return line;
}

// 按 USB VID/PID 在 sysfs 中查找串口节点（ttyACM*/ttyUSB* 均适用）
std::string findSerialPortByVidPid()
{
    namespace fs = std::filesystem;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/sys/class/tty", ec))
    {
        fs::path dev = fs::canonical(entry.path() / "device", ec);
        if (ec)
            continue;

        // 沿设备路径向上找到含 idVendor 的 USB 设备层
        for (fs::path p = dev; p != p.root_path(); p = p.parent_path())
        {
            if (!fs::exists(p / "idVendor"))
                continue;
            if (readFirstLine(p / "idVendor") == kUsbVid &&
                readFirstLine(p / "idProduct") == kUsbPid)
            {
                return "/dev/" + entry.path().filename().string();
            }
            break;
        }
    }
    return {};
}

} // namespace

int main()
{
    // 初始化日志：配置文件位于项目根目录 config/（进程目录为 bin/，取 ../config/），
    // 日志按 default_logconf.json 配置写入 /unitree/var/log/cloth_service（10MB x 3 异步轮转）。
    bpLogger::getInstance().initialize(
        unitree::common::OsHelper::Instance()->GetProcessDirectory() + "../config/default_logconf.json");
    BP_LOG_I("MAIN", "cloth_service starting");

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    // 服务开机自启时 USB 设备可能尚未枚举完成，最多等待 30 秒
    std::string port;
    for (int i = 0; i < 60 && g_running; ++i)
    {
        port = findSerialPortByVidPid();
        if (!port.empty())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (port.empty())
    {
        BP_LOG_E("MAIN", "Serial device ", kUsbVid, ":", kUsbPid, " not found");
        return 1;
    }
    BP_LOG_I("MAIN", "Serial device found: ", port);

    unitree::robot::ChannelFactory::Instance()->Init(0);
    auto dds = std::make_unique<jq::JqDds>();
    dds->Start();
    BP_LOG_I("MAIN", "DDS enabled");

    JqReceiver receiver;
    receiver.setMergedDataCallback([&dds](const jq::JqMergedData &data)
                                   {
                                       if (dds)
                                           dds->SetData(data);
                                   });

    if (!receiver.open(port, kBaudrate))
    {
        BP_LOG_E("MAIN", "Failed to open serial port: ", port);
        return 1;
    }

    BP_LOG_I("MAIN", "Serial port opened: ", port, " @ ", kBaudrate);

    while (g_running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    receiver.close();
    BP_LOG_I("MAIN", "Program exited");

    return 0;
}
