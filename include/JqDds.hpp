#ifndef __JQ_DDS_HPP__
#define __JQ_DDS_HPP__

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "unitree/robot/channel/channel_publisher.hpp"
#include "unitree/idl/ros2/PointCloud2_.hpp"
#include "JqProtocol.h"

namespace jq {

// DDS message layout (reuses sensor_msgs::msg::dds_::PointCloud2_,
// only the data() byte array is used):
//   data_[0..15]   : gyro quaternion, 4 floats (w, x, y, z), little-endian
//   data_[16..271] : 256 sensor data, 8 bits each
class JqDds {
public:
    JqDds();
    ~JqDds();

    void Start();
    void Stop();

    // Called from the serial receive thread with the latest merged frame.
    void SetData(const JqMergedData& data);

private:
    void Run();

    static constexpr size_t GYRO_BYTES = 16;   // 4 floats
    static constexpr size_t SENSOR_BYTES = 256;
    static constexpr size_t PAYLOAD_BYTES = GYRO_BYTES + SENSOR_BYTES;

    std::shared_ptr<unitree::robot::ChannelPublisher<sensor_msgs::msg::dds_::PointCloud2_>> publisher_;
    std::atomic<bool> running_{false};

    std::mutex mutex_;
    sensor_msgs::msg::dds_::PointCloud2_ data_;
    bool hasData_ = false;
    std::thread thread_;
};

} // namespace jq

#endif
