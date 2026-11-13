#include "JqDds.hpp"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "unitree/robot/channel/channel_factory.hpp"

namespace jq {

namespace {
constexpr double PUBLISH_PERIOD_S = 0.01;  // 100 Hz
const char* TOPIC_JQ_CLOTH_STATE = "rt/jq_clothstate";
}

JqDds::JqDds(const std::string& netInterface, int domain) {
    unitree::robot::ChannelFactory::Instance()->Init(domain, netInterface);
    publisher_ = std::make_shared<unitree::robot::ChannelPublisher<sensor_msgs::msg::dds_::PointCloud2_>>(TOPIC_JQ_CLOTH_STATE);
    publisher_->InitChannel();

    // Fixed payload length, allocated once.
    data_.data().resize(PAYLOAD_BYTES);
}

JqDds::~JqDds() {
    Stop();
}

void JqDds::SetData(const JqMergedData& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    const float quat[4] = {data.gyro.w, data.gyro.x, data.gyro.y, data.gyro.z};
    std::memcpy(data_.data().data(), quat, GYRO_BYTES);
    for (size_t i = 0; i < SENSOR_BYTES; ++i) {
        data_.data()[GYRO_BYTES + i] = static_cast<uint8_t>(data.sensor[i]);
    }
    hasData_ = true;
}

void JqDds::Start() {
    running_ = true;
    thread_ = std::thread(&JqDds::Run, this);
    std::cout << "[DDS] Publisher started, topic: " << TOPIC_JQ_CLOTH_STATE << " @ 100Hz" << std::endl;
}

void JqDds::Stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void JqDds::Run() {
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (hasData_) {
                publisher_->Write(data_);
            }
        }
        std::this_thread::sleep_for(std::chrono::duration<double>(PUBLISH_PERIOD_S));
    }
}

} // namespace jq
