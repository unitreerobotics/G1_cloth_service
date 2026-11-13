#pragma once

#include "jq_receiver_export.h"
#include "SerialPort.h"
#include "JqProtocol.h"

#include <string>
#include <vector>
#include <mutex>
#include <functional>

class JQ_RECEIVER_API JqReceiver {
public:
    using MergedDataCallback = std::function<void(const jq::JqMergedData&)>;

    JqReceiver();
    ~JqReceiver();

    bool open(const std::string& port, int baudrate);
    void close();
    bool isOpen() const;

    void setMergedDataCallback(MergedDataCallback callback);

    // Print one merged frame (256 sensor data + 16 gyro data) to stdout.
    static void printMergedData(const jq::JqMergedData& data);

private:
    void onRawData(const std::vector<uint8_t>& chunk);

    SerialPort m_serial;
    MergedDataCallback m_callback;

    std::mutex m_mutex;
    std::vector<uint8_t> m_rxBuffer;
    bool m_hasFirstPacket = false;
    jq::JqFrame m_firstPacket;
};
