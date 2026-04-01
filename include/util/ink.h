#pragma once
#include <string>

namespace custom {
    class ink {
    public:
        // Virtual destructor is vital for cleanup in inheritance
        virtual ~ink() = default;

        // Pure virtual function
        virtual void write(const std::string& message) = 0;
    };
}