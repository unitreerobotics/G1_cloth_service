#include "JqReceiver.h"
#include <memory>
#include "JqDds.hpp"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

void initConsole()
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

void printHelp()
{
    std::cout << "\n========== JQ Clothes Receiver Help ==========" << std::endl;
    std::cout << "The program receives frames from the serial port:" << std::endl;
    std::cout << "  [0xAA 0x55 0x03 0x99] + [seq] + [sensor type] + [data]" << std::endl;
    std::cout << "  seq 0x01: first packet,  128 sensor data" << std::endl;
    std::cout << "  seq 0x02: second packet, 128 sensor data + 16 gyro data" << std::endl;
    std::cout << "Every two packets are merged into one frame" << std::endl;
    std::cout << "(272 int = 256 sensor data + 16 gyro data) and printed." << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  help   - Show this help" << std::endl;
    std::cout << "  quit   - Exit program" << std::endl;
    std::cout << "==============================================" << std::endl;
}

int main()
{
    initConsole();

    std::cout << "========================================" << std::endl;
    std::cout << "     JQ Clothes Receiver Program        " << std::endl;
    std::cout << "     (Two-packet merge mode)            " << std::endl;
    std::cout << "========================================" << std::endl;

    std::string port;
#ifdef _WIN32
    const char* portHint = "COM19";
    std::cout << "Enter COM port (e.g. COM19): ";
#else
    const char* portHint = "/dev/ttyUSB0";
    std::cout << "Enter serial port (e.g. /dev/ttyUSB0): ";
#endif
    std::getline(std::cin, port);

    if (port.empty())
    {
        port = portHint;
        std::cout << "Using default: " << port << std::endl;
    }

    std::string baudStr;
    std::cout << "Enter baudrate (default 921600): ";
    std::getline(std::cin, baudStr);

    int baudrate = 921600;
    if (!baudStr.empty())
    {
        try
        {
            baudrate = std::stoi(baudStr);
        }
        catch (...)
        {
            std::cout << "Invalid baudrate, using default 921600" << std::endl;
        }
    }

    std::string netInterface;
    std::cout << "Enter DDS network interface (default enp2s0, empty to skip DDS): ";
    std::getline(std::cin, netInterface);

    std::unique_ptr<jq::JqDds> dds;
    if (!netInterface.empty())
    {
        dds = std::make_unique<jq::JqDds>(netInterface);
        dds->Start();
    }

    JqReceiver receiver;
    receiver.setMergedDataCallback([&dds](const jq::JqMergedData &data)
                                   {
                                       if (dds)
                                           dds->SetData(data);
                                   });

    if (!receiver.open(port, baudrate))
    {
        std::cerr << "Initialization failed, exiting" << std::endl;
#ifdef _WIN32
        system("pause");
#endif
        return 1;
    }

    printHelp();

    std::string input;
    while (true)
    {
        std::cout << "\n> ";
        std::getline(std::cin, input);

        if (input.empty())
            continue;

        if (input == "quit" || input == "exit")
        {
            break;
        }

        if (input == "help")
        {
            printHelp();
            continue;
        }

        std::cout << "Invalid command, type 'help' for help" << std::endl;
    }

    receiver.close();
    std::cout << "\nProgram exited" << std::endl;

    return 0;
}
