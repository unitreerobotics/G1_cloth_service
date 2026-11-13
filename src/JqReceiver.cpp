#include "JqReceiver.h"

#include <iostream>
#include <iomanip>

JqReceiver::JqReceiver() = default;

JqReceiver::~JqReceiver() {
    close();
}

bool JqReceiver::open(const std::string& port, int baudrate) {
    if (!m_serial.open(port, baudrate)) {
        return false;
    }
    m_serial.startAsyncRead([this](const std::vector<uint8_t>& chunk) {
        onRawData(chunk);
    });
    return true;
}

void JqReceiver::close() {
    m_serial.close();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rxBuffer.clear();
    m_hasFirstPacket = false;
}

bool JqReceiver::isOpen() const {
    return m_serial.isOpen();
}

void JqReceiver::setMergedDataCallback(MergedDataCallback callback) {
    m_callback = callback;
}

void JqReceiver::onRawData(const std::vector<uint8_t>& chunk) {
    std::vector<jq::JqMergedData> mergedList;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rxBuffer.insert(m_rxBuffer.end(), chunk.begin(), chunk.end());

        auto frames = jq::extractFrames(m_rxBuffer);
        for (const auto& frame : frames) {
            if (frame.seq == jq::SEQ_FIRST) {
                m_firstPacket = frame;
                m_hasFirstPacket = true;
            }
            else if (frame.seq == jq::SEQ_SECOND && m_hasFirstPacket) {
                jq::JqMergedData merged;
                if (jq::mergeFrames(m_firstPacket, frame, merged)) {
                    mergedList.push_back(std::move(merged));
                }
                m_hasFirstPacket = false;
            }
        }
    }

    for (const auto& merged : mergedList) {
        if (m_callback) {
            m_callback(merged);
        }
        printMergedData(merged);
    }
}

void JqReceiver::printMergedData(const jq::JqMergedData& data) {
    std::cout << "=======================================================" << std::endl;
    std::cout << "Sensor type: 0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(data.sensorType) << std::dec
              << "  " << jq::sensorTypeName(data.sensorType) << std::endl;

    std::cout << "Sensor data (256 values, 8 bits each):" << std::endl;
    for (size_t i = 0; i < jq::SENSOR_DATA_LEN; ++i) {
        if (i % 16 == 0) {
            std::cout << "  [" << std::setw(3) << std::setfill('0') << i << "] ";
        }
        std::cout << std::setw(4) << std::setfill(' ') << data.sensor[i];
        if (i % 16 == 15) {
            std::cout << std::endl;
        }
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Gyro quaternion (16 bytes -> W X Y Z):" << std::endl;
    std::cout << "  W = " << data.gyro.w << std::endl;
    std::cout << "  X = " << data.gyro.x << std::endl;
    std::cout << "  Y = " << data.gyro.y << std::endl;
    std::cout << "  Z = " << data.gyro.z << std::endl;
    std::cout << "=======================================================" << std::endl;
}
