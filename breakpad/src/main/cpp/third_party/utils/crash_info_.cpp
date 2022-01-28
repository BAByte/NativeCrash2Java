//
// Created by ba on 1/25/22.
//

#include <cstdint>
#include <common/linux/linux_libc_support.h>
#include "crash_info_.h"

babyte::CrashInfo::CrashInfo(google_breakpad::LinuxDumper *dumper, size_t size) {
    dumper_ = dumper;
    kLineBufferSize = size;
    log_line_ = reinterpret_cast<char *>(Alloc(size));
    log_line_[0] = '\0';
};

babyte::CrashInfo::~CrashInfo() {
    clear();
    log_line_ = nullptr;
    dumper_ = nullptr;
}

void babyte::CrashInfo::clear() {
    if (log_line_) {
        log_line_[0] = 0;
    }
}

void babyte::CrashInfo::append(char *str) {
    append(const_cast<const char *>(str));
}

void babyte::CrashInfo::append(int i) {
    char *buf = new char[sizeof(i) + 1];
    sprintf(buf, "%d", i);
    append(buf);
}

void babyte::CrashInfo::append(long i) {
    char *buf = new char[sizeof(i) + 1];
    sprintf(buf, "%ld", i);
    append(buf);
}

// Stages the given string in the current line buffer.
void babyte::CrashInfo::append(const char *str) {
    my_strlcat(babyte::CrashInfo::log_line_, str, kLineBufferSize);
}

// Stages the buffer content hex-encoded in the current line buffer.
void babyte::CrashInfo::append(const void *buf, size_t length) {
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(buf);
    for (size_t i = 0; i < length; ++i, ++ptr)
        append(*ptr);
}