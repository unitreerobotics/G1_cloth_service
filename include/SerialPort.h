#pragma once

#include "jq_receiver_export.h"

#include <string>
#include <vector>
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <chrono>

#include <termios.h>

class JQ_RECEIVER_API SerialPort {
public:
    using DataReceivedCallback = std::function<void(const std::vector<uint8_t>&)>;

    SerialPort();
    ~SerialPort();

    bool open(const std::string& port, int baudrate, double timeout = 0.1);
    void close();
    bool isOpen() const;
    void write(const std::vector<uint8_t>& data);
    std::vector<uint8_t> read(size_t size);
    void flush();

    // Async receive (raw bytes, protocol parsing is left to the consumer)
    void startAsyncRead(DataReceivedCallback callback);
    void stopAsyncRead();

private:
    int m_fd;
    std::atomic<bool> m_isOpen;
    std::atomic<bool> m_running;
    std::thread m_readThread;
    DataReceivedCallback m_callback;
    double m_timeoutSec;

    std::mutex m_bufferMutex;
    std::vector<uint8_t> m_buffer;

    void configurePort(int baudrate, double timeout);
    void readThreadFunc();
    bool readAvailableData();
    void processBuffer();
};
