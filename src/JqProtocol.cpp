#include "JqProtocol.h"

#include <cstring>

namespace jq {

const char* sensorTypeName(uint8_t type) {
    switch (type) {
    case 0x01: return "LH (Left Hand)";
    case 0x02: return "RH (Right Hand)";
    case 0x03: return "LF (Left Foot)";
    case 0x04: return "RF (Right Foot)";
    case 0x05: return "WB (Whole Body)";
    default:   return "Unknown";
    }
}

// Expected total frame length given the packet-sequence byte.
// Returns 0 for an invalid sequence byte.
static size_t frameLengthForSeq(uint8_t seq) {
    if (seq == SEQ_FIRST)  return FIRST_FRAME_LEN;
    if (seq == SEQ_SECOND) return SECOND_FRAME_LEN;
    return 0;
}

static bool isHeaderAt(const std::vector<uint8_t>& buf, size_t pos) {
    if (buf.size() - pos < HEADER_LEN) {
        // Not enough bytes yet; only trust what we can compare.
        for (size_t i = 0; i < buf.size() - pos; ++i) {
            if (buf[pos + i] != HEADER[i]) return false;
        }
        return true;
    }
    for (size_t i = 0; i < HEADER_LEN; ++i) {
        if (buf[pos + i] != HEADER[i]) return false;
    }
    return true;
}

std::vector<JqFrame> extractFrames(std::vector<uint8_t>& buffer) {
    std::vector<JqFrame> frames;

    while (buffer.size() >= HEADER_LEN + 2) {
        // 1. Locate the frame header.
        size_t headerPos = 0;
        bool found = false;
        for (size_t i = 0; i + HEADER_LEN <= buffer.size(); ++i) {
            if (isHeaderAt(buffer, i)) {
                headerPos = i;
                found = true;
                break;
            }
        }

        if (!found) {
            // Keep the last (HEADER_LEN - 1) bytes in case the header is split.
            buffer.erase(buffer.begin(), buffer.end() - (HEADER_LEN - 1));
            break;
        }

        if (headerPos > 0) {
            buffer.erase(buffer.begin(), buffer.begin() + headerPos);
        }

        // 2. Need the seq byte to know the frame length.
        if (buffer.size() < HEADER_LEN + 1) {
            break;
        }

        uint8_t seq = buffer[HEADER_LEN];
        size_t frameLen = frameLengthForSeq(seq);
        if (frameLen == 0) {
            // Invalid seq byte: drop this header and resync.
            buffer.erase(buffer.begin(), buffer.begin() + 1);
            continue;
        }

        // 3. Wait for the complete frame.
        if (buffer.size() < frameLen) {
            break;
        }

        JqFrame frame;
        frame.seq = seq;
        frame.sensorType = buffer[HEADER_LEN + 1];
        frame.data.assign(buffer.begin() + HEADER_LEN + 2,
                          buffer.begin() + frameLen);
        frames.push_back(std::move(frame));

        buffer.erase(buffer.begin(), buffer.begin() + frameLen);
    }

    // Guard against unbounded growth on garbage input.
    if (buffer.size() > 4096) {
        buffer.erase(buffer.begin(), buffer.end() - (HEADER_LEN - 1));
    }

    return frames;
}

bool mergeFrames(const JqFrame& first, const JqFrame& second, JqMergedData& out) {
    if (first.seq != SEQ_FIRST || second.seq != SEQ_SECOND) {
        return false;
    }
    if (first.data.size() != FIRST_DATA_LEN || second.data.size() != SECOND_DATA_LEN) {
        return false;
    }
    if (first.sensorType != second.sensorType) {
        return false;
    }

    out.sensorType = first.sensorType;

    // Combine the two 128-byte sensor data halves into one 256-value array
    // (each value is 8 bits, 0~255).
    for (size_t i = 0; i < FIRST_DATA_LEN; ++i) {
        out.sensor[i] = static_cast<int>(first.data[i]);
    }
    for (size_t i = 0; i < FIRST_DATA_LEN; ++i) {
        out.sensor[FIRST_DATA_LEN + i] = static_cast<int>(second.data[i]);
    }

    // The remaining 16 bytes of the second packet are gyroscope data:
    // every 4 bytes (little-endian IEEE754 float) combine into one
    // quaternion component, forming Quaternion(q0, q1, q2, q3).
    const uint8_t* gyroBytes = second.data.data() + FIRST_DATA_LEN;
    float* q = &out.gyro.w;
    for (size_t i = 0; i < GYRO_DATA_LEN / 4; ++i) {
        uint32_t raw = static_cast<uint32_t>(gyroBytes[i * 4 + 0])        |
                      (static_cast<uint32_t>(gyroBytes[i * 4 + 1]) << 8)  |
                      (static_cast<uint32_t>(gyroBytes[i * 4 + 2]) << 16) |
                      (static_cast<uint32_t>(gyroBytes[i * 4 + 3]) << 24);
        std::memcpy(&q[i], &raw, sizeof(float));
    }

    return true;
}

} // namespace jq
