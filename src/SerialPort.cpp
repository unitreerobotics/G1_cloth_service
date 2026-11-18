#include "SerialPort.h"
#include <stdexcept>
#include <chrono>
#include <cstring>

#include "utils/bp_logger.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#ifdef __linux__
#include <linux/serial.h>
#if defined(TCGETS2) && !defined(BOTHER)
struct termios2 {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_line;
    cc_t c_cc[19];
    speed_t c_ispeed;
    speed_t c_ospeed;
};
#define BOTHER 0x00001000
#endif
#endif

SerialPort::SerialPort()
    : m_fd(-1)
    , m_isOpen(false)
    , m_running(false)
    , m_timeoutSec(0.1) {
}

SerialPort::~SerialPort() {
    close();
}

bool SerialPort::open(const std::string& port, int baudrate, double timeout) {
    if (m_isOpen) {
        close();
    }

    m_timeoutSec = timeout;

    m_fd = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (m_fd < 0) {
        BP_LOG_E("SERIAL", "Failed to open port ", port, ": ", strerror(errno));
        return false;
    }

    try {
        configurePort(baudrate, timeout);
        m_isOpen = true;
        BP_LOG_I("SERIAL", "Port ", port, " opened, baudrate: ", baudrate);
        return true;
    }
    catch (const std::exception& e) {
        BP_LOG_E("SERIAL", "Port configuration failed: ", e.what());
        ::close(m_fd);
        m_fd = -1;
        return false;
    }
}

void SerialPort::close() {
    stopAsyncRead();
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
        m_isOpen = false;
    }
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffer.clear();
    }
}

bool SerialPort::isOpen() const {
    return m_isOpen;
}

void SerialPort::configurePort(int baudrate, double timeout) {
    (void)timeout;

    struct termios tty {};
    if (tcgetattr(m_fd, &tty) != 0) {
        throw std::runtime_error(std::string("tcgetattr failed: ") + strerror(errno));
    }

    bool non_standard_baud = false;
    speed_t baud;
    switch (baudrate) {
    case 9600:    baud = B9600;    break;
    case 19200:   baud = B19200;   break;
    case 38400:   baud = B38400;   break;
    case 57600:   baud = B57600;   break;
    case 115200:  baud = B115200;  break;
    case 230400:  baud = B230400;  break;
    case 460800:  baud = B460800;  break;
    case 500000:  baud = B500000;  break;
    case 576000:  baud = B576000;  break;
    case 921600:  baud = B921600;  break;
    case 1000000: baud = B1000000; break;
    case 1152000: baud = B1152000; break;
    case 1500000: baud = B1500000; break;
    case 2000000: baud = B2000000; break;
    case 2500000: baud = B2500000; break;
    case 3000000: baud = B3000000; break;
    case 3500000: baud = B3500000; break;
    case 4000000: baud = B4000000; break;
    default:
        baud = B38400;
        non_standard_baud = true;
        break;
    }

    cfsetispeed(&tty, baud);
    cfsetospeed(&tty, baud);

    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS | HUPCL);
    tty.c_cflag |= (CLOCAL | CREAD);

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL |
                     IXON | IXOFF | IXANY | IMAXBEL);
    tty.c_iflag |= IGNPAR;
    tty.c_oflag &= ~(OPOST | ONLCR | OCRNL | ONOCR | ONLRET);
    tty.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL | IEXTEN);

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        throw std::runtime_error(std::string("tcsetattr failed: ") + strerror(errno));
    }

#ifdef __linux__
    struct serial_struct sc {};
    if (ioctl(m_fd, TIOCGSERIAL, &sc) == 0) {
        sc.flags |= ASYNC_LOW_LATENCY;
        if (non_standard_baud && sc.baud_base >= static_cast<int>(baudrate) && baudrate > 0) {
            sc.flags &= ~ASYNC_SPD_MASK;
            sc.flags |= ASYNC_SPD_CUST;
            sc.custom_divisor = sc.baud_base / baudrate;
        }
        ioctl(m_fd, TIOCSSERIAL, &sc);
    }

#if defined(TCGETS2)
    if (non_standard_baud) {
        struct termios2 t2 {};
        if (ioctl(m_fd, TCGETS2, &t2) == 0) {
            t2.c_cflag &= ~CBAUD;
            t2.c_cflag |= BOTHER;
            t2.c_ispeed = baudrate;
            t2.c_ospeed = baudrate;
            if (ioctl(m_fd, TCSETS2, &t2) != 0) {
                throw std::runtime_error(
                    std::string("TCSETS2 failed for baudrate ") +
                    std::to_string(baudrate) + ": " + strerror(errno));
            }
        }
    }
#endif
#else
    if (non_standard_baud) {
        throw std::runtime_error(
            "Non-standard baudrate requires Linux termios2 support");
    }
#endif

    // Clear O_NONBLOCK for more predictable write/read; async path uses select.
    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(m_fd, F_SETFL, flags & ~O_NONBLOCK);
    }

    tcflush(m_fd, TCIOFLUSH);
}

void SerialPort::write(const std::vector<uint8_t>& data) {
    if (!m_isOpen) {
        throw std::runtime_error("Port not open");
    }

    size_t total = 0;
    while (total < data.size()) {
        ssize_t n = ::write(m_fd, data.data() + total, data.size() - total);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(std::string("Port write failed: ") + strerror(errno));
        }
        if (n == 0) {
            throw std::runtime_error("Port write incomplete");
        }
        total += static_cast<size_t>(n);
    }
}

std::vector<uint8_t> SerialPort::read(size_t size) {
    if (!m_isOpen) {
        throw std::runtime_error("Port not open");
    }

    std::vector<uint8_t> buffer(size);

    size_t totalRead = 0;
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                        std::chrono::duration<double>(m_timeoutSec));

    while (totalRead < size) {
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            break;
        }

        auto remain = std::chrono::duration_cast<std::chrono::microseconds>(deadline - now);
        timeval tv {};
        tv.tv_sec = static_cast<time_t>(remain.count() / 1000000);
        tv.tv_usec = static_cast<suseconds_t>(remain.count() % 1000000);

        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(m_fd, &rset);

        int sel = select(m_fd + 1, &rset, nullptr, nullptr, &tv);
        if (sel < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(std::string("Port read select failed: ") + strerror(errno));
        }
        if (sel == 0) {
            break;
        }

        ssize_t n = ::read(m_fd, buffer.data() + totalRead, size - totalRead);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            throw std::runtime_error(std::string("Port read failed: ") + strerror(errno));
        }
        if (n == 0) {
            break;
        }
        totalRead += static_cast<size_t>(n);
    }

    buffer.resize(totalRead);
    return buffer;
}

void SerialPort::flush() {
    if (m_isOpen) {
        tcflush(m_fd, TCIOFLUSH);
    }
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffer.clear();
    }
}

void SerialPort::startAsyncRead(DataReceivedCallback callback) {
    if (!m_isOpen) {
        throw std::runtime_error("Port not open");
    }

    if (m_running) {
        stopAsyncRead();
    }

    m_callback = callback;
    m_running = true;
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffer.clear();
    }
    m_readThread = std::thread(&SerialPort::readThreadFunc, this);
}

void SerialPort::stopAsyncRead() {
    m_running = false;
    if (m_readThread.joinable()) {
        m_readThread.join();
    }
    m_callback = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffer.clear();
    }
}

void SerialPort::readThreadFunc() {
    while (m_running) {
        if (readAvailableData()) {
            processBuffer();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

bool SerialPort::readAvailableData() {
    if (!m_isOpen) {
        return false;
    }

    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(m_fd, &rset);
    timeval tv {};
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int sel = select(m_fd + 1, &rset, nullptr, nullptr, &tv);
    if (sel <= 0) {
        return false;
    }

    uint8_t temp[1024];
    ssize_t n = ::read(m_fd, temp, sizeof(temp));
    if (n <= 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_buffer.insert(m_buffer.end(), temp, temp + n);
    return true;
}

void SerialPort::processBuffer() {
    // Deliver all accumulated raw bytes; protocol parsing is left to the consumer.
    std::vector<uint8_t> chunk;
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        if (m_buffer.empty()) {
            return;
        }
        chunk.swap(m_buffer);
    }

    if (m_callback) {
        m_callback(chunk);
    }
}
