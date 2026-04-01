#pragma once
#include "ink.h"

namespace custom {
    class console_ink : public ink {
    public:
        void write(const std::string& message) override;
    };
}