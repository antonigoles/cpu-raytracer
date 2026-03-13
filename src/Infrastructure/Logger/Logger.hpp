#pragma once
#include <thread>
#include <Infrastructure/Logger/Logger.hpp>
#include <iostream>

#define ENABLE_VERBOSE true

enum class LogLevel {
    VERBOSE,
    INFO,
    WARNING,
    ERR
};

inline std::mutex log_mutex;
template <typename... Args>
void log(LogLevel level, Args&&... args) {
    std::lock_guard<std::mutex> guard(log_mutex);

    switch (level) {
        case LogLevel::VERBOSE:
            if (ENABLE_VERBOSE) std::cout << "\033[32m[VERBOSE]\033[0m ";
            break;
        case LogLevel::INFO:
            std::cout << "\033[32m[INFO]\033[0m ";
            break;
        case LogLevel::WARNING:
            std::cout << "\033[33m[WARN]\033[0m ";
            break;
        case LogLevel::ERR:
            std::cerr << "\033[31m[ERROR]\033[0m ";
            (std::cerr << ... << std::forward<Args>(args)) << std::endl;
            return;
    }
    
    (std::cout << ... << std::forward<Args>(args)) << std::endl;
}

// Wygodne wrappery, żeby pisać jeszcze mniej kodu:
template <typename... Args> void log_verbose(Args&&... args) { log(LogLevel::VERBOSE, std::forward<Args>(args)...); }
template <typename... Args> void log_info(Args&&... args) { log(LogLevel::INFO, std::forward<Args>(args)...); }
template <typename... Args> void log_warn(Args&&... args) { log(LogLevel::WARNING, std::forward<Args>(args)...); }
template <typename... Args> void log_err(Args&&... args)  { log(LogLevel::ERR, std::forward<Args>(args)...); }