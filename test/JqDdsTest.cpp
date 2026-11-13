// DDS subscriber test for topic "rt/jq_clothstate".
// Usage: JqDdsTest [network_interface]   (default: lo)
// Prints gyro quaternion, a few sensor values and the receive rate.

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "unitree/robot/channel/channel_factory.hpp"
#include "unitree/robot/channel/channel_subscriber.hpp"
#include "unitree/idl/ros2/PointCloud2_.hpp"

using sensor_msgs::msg::dds_::PointCloud2_;

namespace {
std::atomic<uint64_t> msgCount{0};
PointCloud2_ lastMsg;
std::mutex lastMsgMutex;
bool hasMsg = false;
} // namespace

void onClothState(const void* msg)
{
    std::lock_guard<std::mutex> lock(lastMsgMutex);
    lastMsg = *static_cast<const PointCloud2_*>(msg);
    hasMsg = true;
    ++msgCount;
}

int main(int argc, char* argv[])
{
    const std::string netInterface = (argc > 1) ? argv[1] : "lo";

    unitree::robot::ChannelFactory::Instance()->Init(0, netInterface);

    auto subscriber = std::make_shared<unitree::robot::ChannelSubscriber<PointCloud2_>>("rt/jq_clothstate");
    subscriber->InitChannel(onClothState, 1);

    std::cout << "Subscribed to rt/jq_clothstate on interface " << netInterface << std::endl;
    std::cout << "Waiting for messages... (Ctrl+C to quit)" << std::endl;

    uint64_t lastCount = 0;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        uint64_t count = msgCount.load();
        double rate = static_cast<double>(count - lastCount);  // per second
        lastCount = count;
        if (count == 0)
        {
            continue;
        }

        PointCloud2_ m;
        {
            std::lock_guard<std::mutex> lock(lastMsgMutex);
            m = lastMsg;
        }

        std::cout << "recv " << count << " msgs, rate " << rate << " Hz, payload "
                  << m.data().size() << " bytes" << std::endl;
        if (m.data().size() >= 272)
        {
            float quat[4];
            std::memcpy(quat, m.data().data(), 16);
            const uint8_t* sensor = m.data().data() + 16;

            std::cout << "Gyro quaternion: W=" << quat[0] << " X=" << quat[1]
                      << " Y=" << quat[2] << " Z=" << quat[3] << std::endl;
            std::cout << "Sensor data (256 values, 8 bits each):" << std::endl;
            for (int i = 0; i < 256; ++i)
            {
                if (i % 8 == 0)
                    std::printf("  [%3d] ", i);
                std::printf("%4d", sensor[i]);
                if (i % 8 == 7)
                    std::cout << std::endl;
            }
        }
    }

    return 0;
}
