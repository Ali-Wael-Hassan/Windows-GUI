#include "util/logger.h"
#include <mutex>

namespace custom {
    static std::mutex logMutex;

    logger& logger::getInstance() {
        static logger instance;
        return instance;
    }

    void logger::add_sink(std::unique_ptr<ink> s) {
        std::lock_guard<std::mutex> lock(logMutex);
        sinks.push_back(std::move(s));
    }

    void logger::log(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        for (auto& sink : sinks) {
            if (sink) sink->write(message); 
        }
    }
}