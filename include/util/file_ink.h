#pragma once
#include "util/ink.h" // Note: Make sure path is correct
#include <fstream>
#include <string>

namespace custom {
    class file_ink : public ink {
    public:
        explicit file_ink(const std::string& filename);
        ~file_ink() override;
        void write(const std::string& message) override;

    private:
        std::ofstream m_file;
    };
}