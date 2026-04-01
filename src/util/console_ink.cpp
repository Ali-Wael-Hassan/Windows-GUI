#include "util/console_ink.h"
#include <iostream>

namespace custom {
    void console_ink::write(const std::string& message) {
        std::cout << "[CONSOLE]: " << message << std::endl;
    }
}