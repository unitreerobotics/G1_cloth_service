/*
 * @file bp_logger.cpp
 * @brief 基础平台日志系统实现文件（baseplatform）
 * 
 * 基于 unitree_sdk2 日志系统封装的通用日志系统实现，提供跨平台的
 * 日志记录功能。支持文件输出、控制台输出、多级别过滤、
 * 异步输出等特性。
 * 
 * @author Base Platform Team
 * @version 2.0.0
 * @date 2025-11-24
 * 
 * @implementation_details
 * - 采用单例模式确保全局唯一实例
 * - 使用 unitree_sdk2 日志系统作为底层日志引擎
 * - 支持配置文件和代码配置两种方式
 * - 实现文件自动轮转功能
 * - 提供线程安全的日志输出
 * 
 * @dependencies
 * - unitree_sdk2 log module
 * - C++14 标准库
 */

#include "utils/bp_logger.hpp"
#include <mutex>
#include <iostream>

using namespace unitree::common;

// 初始化静态成员
unitree::common::Logger* bpLogger::default_logger_ = nullptr;

// 静态变量
static std::mutex logger_mutex;

bpLogger::bpLogger() 
    : initialized_(false) {
}

bpLogger::~bpLogger() {
    deinitialize();
}

bpLogger& bpLogger::getInstance() {
    static bpLogger instance;
    return instance;
}

bool bpLogger::initialize(const std::string& config_file_path) {
    std::lock_guard<std::mutex> lock(logger_mutex);
    
    if (initialized_) {
        return true;
    }

    try {
        #if ENABLE_DEBUG_MODE
            bool is_enable_stdout = true;
        #else
            bool is_enable_stdout = false;
        #endif

        if (!config_file_path.empty()) {
            // 使用配置文件初始化
            LogInit(config_file_path, is_enable_stdout);
        } else {
            // 使用默认配置（仅stdout）
            LogInit("", is_enable_stdout);
        }
        
        // 获取默认logger
        default_logger_ = GetLogger("/unitree");
        if (!default_logger_) {
            std::cerr << "Failed to get default logger" << std::endl;
            return false;
        }
        
        initialized_ = true;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        return false;
    }
}

void bpLogger::deinitialize() {
    std::lock_guard<std::mutex> lock(logger_mutex);
    
    if (!initialized_) {
        return;
    }

    try {
        // 清空缓存（只是清除指针,不释放内存）
        default_logger_ = nullptr;
        
        // 调用 unitree_sdk2 的清理函数
        LogFinal();
    } catch (...) {
        // 忽略析构时的异常
    }
    
    initialized_ = false;
}

bool bpLogger::isInitialized() const {
    return initialized_;
}

