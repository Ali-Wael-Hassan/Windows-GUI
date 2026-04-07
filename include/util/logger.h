#pragma once
#include <vector>
#include <memory>
#include <string>
#include "util/ink.h"

#define LOG(msg) custom::logger::getInstance().log(msg);

namespace custom {
    class logger {
    private:
        logger() = default; 
        std::vector<std::unique_ptr<ink>> sinks;
    public:
        static logger& getInstance();

        void add_sink(std::unique_ptr<ink> s);
        void log(const std::string& message);

        logger(const logger&) = delete;
        logger& operator=(const logger&) = delete;
    };
}