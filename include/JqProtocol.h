#pragma once

#include "jq_receiver_export.h"

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>

/**
 * JQ clothes serial protocol:
 *   frame = [header: 0xAA 0x55 0x03 0x99] + [packet seq] + [sensor type] + [data]
 *
 *   packet seq : 0x01 -> first packet,  data length = 128 (first 128 sensor data)
 *                0x02 -> second packet, data length = 144 (last 128 sensor data
 *                        + 16 gyroscope data)
 *   sensor type: 0x01 LH (Left Hand)   0x02 RH (Right Hand)
 *                0x03 LF (Left Foot)   0x04 RF (Right Foot)
 *                0x05 WB (Whole Body)
 */
namespace jq {

constexpr uint8_t HEADER[4] = {0xAA, 0x55, 0x03, 0x99};
constexpr size_t HEADER_LEN = 4;

constexpr uint8_t SEQ_FIRST = 0x01;
constexpr uint8_t SEQ_SECOND = 0x02;

constexpr size_t FIRST_DATA_LEN = 128;   // first 128 sensor data
constexpr size_t SECOND_DATA_LEN = 144;  // last 128 sensor data + 16 gyro data
constexpr size_t SENSOR_DATA_LEN = 256;  // 128 + 128
constexpr size_t GYRO_DATA_LEN = 16;

constexpr size_t FIRST_FRAME_LEN = HEADER_LEN + 1 + 1 + FIRST_DATA_LEN;   // 134
constexpr size_t SECOND_FRAME_LEN = HEADER_LEN + 1 + 1 + SECOND_DATA_LEN; // 150

// One complete frame (first or second packet)
struct JqFrame {
    uint8_t seq = 0;         // packet sequence (0x01 / 0x02)
    uint8_t sensorType = 0;  // sensor type (0x01 ~ 0x05)
    std::vector<uint8_t> data;
};

// Two packets merged into one: 256 sensor data (8 bits each, 0~255)
// + 16 gyro bytes combined into a quaternion (4 bytes per component).

// Quaternion constructed from 4 components,
// each component combined from 4 bytes of the gyro data (little-endian float).
struct Quaternion {
    float w = 0.0f;  // q0
    float x = 0.0f;  // q1
    float y = 0.0f;  // q2
    float z = 0.0f;  // q3
};

struct JqMergedData {
    uint8_t sensorType = 0;
    std::array<int, SENSOR_DATA_LEN> sensor{};  // 256 values, 8 bits each (0~255)
    Quaternion gyro{};                          // 16 gyro bytes -> quaternion
};

const char* sensorTypeName(uint8_t type);

// Extract complete frames from a raw byte stream buffer.
// Consumed bytes are erased from `buffer`; incomplete data is kept.
std::vector<JqFrame> extractFrames(std::vector<uint8_t>& buffer);

// Merge a first packet and a second packet into one JqMergedData.
bool mergeFrames(const JqFrame& first, const JqFrame& second, JqMergedData& out);

} // namespace jq
