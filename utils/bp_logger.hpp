/*
 * @file bp_logger.hpp
 * @brief 基础平台日志系统头文件 (baseplatform)
 * 
 * 基于 unitree_sdk2 日志系统封装的通用日志系统，提供简洁易用的接口，
 * 支持多级别日志输出、文件输出、控制台输出等功能。
 * 适用于各种 C++ 项目的日志记录需求。
 * 
 * @author Base Platform Team
 * @version 2.0.0
 * @date 2025-11-24
 * 
 * @features
 * - 支持多个日志级别：ERROR/WARN/INFO/DEBUG
 * - 支持文件输出和控制台输出
 * - 支持动态调整日志级别和输出目标
 * - 线程安全，支持多线程环境
 * - 异步输出模式，不阻塞主线程
 * - 自动文件轮转，防止日志文件过大
 * - 简洁的宏接口，使用方便
 * - 使用流式输入（逗号分隔参数），而非printf风格
 * 
 * @example
 * @code
 * #include "bp_logger.hpp"
 * 
 * int main() {
 *     bpLogger& logger = bpLogger::getInstance();
 *     logger.initialize("/unitree/var/log/move_service/default_logconf.json");
 *     
 *     // 流式输入，使用逗号分隔参数
 *     BP_LOG_I("MAIN", "应用启动，版本: ", "1.0.0");
 *     BP_LOG_W("SENSOR", "传感器异常，错误码: ", error_code);
 *     BP_LOG_E("SYSTEM", "系统错误: ", error_msg);
 *     
 *     return 0;
 * }
 * @endcode
 */

#ifndef BASEPLATFORM_LOGGER_HPP
#define BASEPLATFORM_LOGGER_HPP

#include <string>
#include <map>
#include <unitree/common/log/log_initor.hpp>


/**
 * @brief 基础平台日志管理器类
 * 
 * 基于 unitree_sdk2 日志系统封装的简洁易用的日志系统，支持文件输出和终端输出。
 * 采用单例模式设计，确保全局唯一的日志实例。
 * 
 * @note 线程安全，支持多线程环境使用
 * @note 支持异步输出模式，不阻塞主线程
 * @note 支持自动文件轮转，防止日志文件过大
 */
class bpLogger {
public:
    /**
     * @brief 构造函数
     */
    bpLogger();
    
    /**
     * @brief 析构函数
     */
    ~bpLogger();

    /**
     * @brief 初始化日志系统
     * 
     * @param config_file_path 日志配置文件路径，如果为空则使用默认配置
     * @param level 日志输出等级
     * @return true 初始化成功
     * @return false 初始化失败
     * 
     * @example
     * @code
     * bpLogger& logger = bpLogger::getInstance();
     * // 使用配置文件初始化
     * logger.initialize("/etc/bp_logconf.json");
     * // 使用默认配置初始化
     * logger.initialize("");
     * @endcode
     */
    bool initialize(const std::string& config_file_path = "");

    /**
     * @brief 反初始化日志系统
     * 
     * 清理资源，关闭文件句柄，停止异步线程等。
     * 通常在程序退出前调用，或需要重新配置日志系统时调用。
     */
    void deinitialize();

    /**
     * @brief 获取单例实例
     * 
     * @return bpLogger& 单例引用
     * 
     * @note 线程安全的单例实现
     */
    static bpLogger& getInstance();

    /**
     * @brief 检查日志系统是否已初始化
     * 
     * @return true 已初始化
     * @return false 未初始化
     */
    bool isInitialized() const;

    /**
     * @brief 默认Logger指针（在initialize时获取，由unitree_sdk2管理生命周期）
     */
    static unitree::common::Logger* default_logger_;

private:
    bool initialized_;                                      ///< 是否已初始化
    
    // 禁用拷贝构造和赋值操作，确保单例模式
    bpLogger(const bpLogger&) = delete;
    bpLogger& operator=(const bpLogger&) = delete;
};

/**
 * @brief 获取默认Logger实例
 * @return unitree::common::Logger* Logger实例指针，未初始化时返回nullptr
 */
inline unitree::common::Logger* bpGetLogger() {
    return bpLogger::default_logger_;
}

/**
 * @brief 格式化tag字符串，使其左对齐到固定宽度
 * @param tag 原始tag字符串
 * @return std::string 格式化后的tag（固定20字符宽度，左对齐）
 */
inline std::string bpFormatTag(const char* tag) {
    std::string formatted = "[";
    formatted += tag;
    formatted += "]";
    // 补齐到23字符（包括[]），超过则截断不补空格
    if (formatted.length() < 23) {
        formatted.append(23 - formatted.length(), ' ');
    }
    return formatted;
}

// ===============================================================================
// 基础平台日志宏定义
// ===============================================================================

/**
 * @defgroup BP_LOG_MACROS 基础平台日志宏
 * @brief 提供便捷的日志输出宏接口
 * 
 * 这些宏提供了简洁易用的日志输出接口，用户只需指定标签和消息内容即可。
 * 
 * @note 重要：使用流式输入（逗号分隔参数），而非printf风格格式化字符串
 * @note 示例：BP_LOG_I("TAG", "Message: ", value) 而不是 BP_LOG_I("TAG", "Message: %d", value)
 * 
 * @{
 */

/**
 * @brief 错误级别日志宏
 * @param tag 日志标签，用于标识日志来源模块（会在日志消息中显示）
 * @param ... 日志消息和参数（使用逗号分隔，流式输入）
 */
#define BP_LOG_E(tag, ...) do { \
    auto* logger = bpGetLogger(); \
    if (logger) { \
        LOG_ERROR(logger, bpFormatTag(tag), __VA_ARGS__); \
    } \
} while(0)

/**
 * @brief 警告级别日志宏
 * @param tag 日志标签，用于标识日志来源模块（会在日志消息中显示）
 * @param ... 日志消息和参数（使用逗号分隔，流式输入）
 */
#define BP_LOG_W(tag, ...) do { \
    auto* logger = bpGetLogger(); \
    if (logger) { \
        LOG_WARNING(logger, bpFormatTag(tag), __VA_ARGS__); \
    } \
} while(0)

/**
 * @brief 信息级别日志宏
 * @param tag 日志标签，用于标识日志来源模块（会在日志消息中显示）
 * @param ... 日志消息和参数（使用逗号分隔，流式输入）
 */
#define BP_LOG_I(tag, ...) do { \
    auto* logger = bpGetLogger(); \
    if (logger) { \
        LOG_INFO(logger, bpFormatTag(tag), __VA_ARGS__); \
    } \
} while(0)

/**
 * @brief 调试级别日志宏
 * @param tag 日志标签，用于标识日志来源模块（会在日志消息中显示）
 * @param ... 日志消息和参数（使用逗号分隔，流式输入）
 */
#define BP_LOG_D(tag, ...) do { \
    auto* logger = bpGetLogger(); \
    if (logger) { \
        LOG_DEBUG(logger, bpFormatTag(tag), __VA_ARGS__); \
    } \
} while(0)



/**
 * @}
 */
#define UT_DEFAULT_LOG_CONF_PATH \
    OsHelper::Instance()->GetProcessDirectory() + "../config/default_logconf.json"


// ===============================================================================
// 使用示例
// ===============================================================================

/**
 * @example
 * @code
 * // 初始化日志系统
 * bpLogger::getInstance().initialize("/etc/bp_logconf.json");
 * 
 * // 使用不同级别的日志 - 注意：使用逗号分隔参数，而非printf格式化
 * // tag 会自动添加到日志消息前面，格式：[tag] 消息内容
 * BP_LOG_I("MAIN", "应用启动，版本: ", version);
 * // 输出: [2025-11-24 11:00:00.000] [INFO] [PID] [TID] [MAIN] 应用启动，版本: 1.0.0
 * 
 * BP_LOG_W("SENSOR", "传感器温度偏高: ", temperature, "°C");
 * // 输出: [2025-11-24 11:00:00.001] [WARNING] [PID] [TID] [SENSOR] 传感器温度偏高: 85°C
 * 
 * BP_LOG_E("DATABASE", "数据库连接失败: ", error_msg);
 * BP_LOG_D("NETWORK", "发送数据包，大小: ", size, " bytes");
 * 
 * // 错误用法（不要使用printf风格）:
 * // BP_LOG_I("TAG", "Value: %d", value);  // ❌ 这样会导致格式字符串被当作普通文本
 * 
 * // 正确用法（使用流式输入）:
 * // BP_LOG_I("TAG", "Value: ", value);     // ✅ 使用逗号分隔
 * @endcode
 */

#endif // BASEPLATFORM_LOGGER_HPP
