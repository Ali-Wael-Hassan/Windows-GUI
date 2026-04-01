#include "util/file_ink.h"
#include <iostream>

namespace custom {

    file_ink::file_ink(const std::string& filename) {
        m_file.open(filename, std::ios::out | std::ios::app);
        
        #ifdef _DEBUG
        if (!m_file.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
        #endif
    }

    file_ink::~file_ink() {
        if (m_file.is_open()) {
            m_file.flush();
            m_file.close();
        }
    }

    void file_ink::write(const std::string& message) {
        if (m_file.is_open()) {
            m_file << message << std::endl;
            
            m_file.flush();
        }
    }
}