//
// Created by ba on 1/25/22.
//

#include <cstdint>
#include <common/linux/linux_libc_support.h>
#include "crash_info_utils.h"

/**
* fomat like
* #05 pc 01a3862c  /data/app/com.babyte.banativecrash-1/oat/x86/base.odex (offset 0xd68000) (void kotlinx.coroutines.DispatchedTask.run()+2560)
* @param trackInfo
* @param line
*/
void babyte::CrashInfo::format(uintptr_t pc, babyte::CrashInfo *trackInfo, int line) {
    Dl_info info;
    char num[16];
    sprintf(num, "%02d", line);
    trackInfo->append("     #");
    trackInfo->append(num);
    if (dladdr((void *) pc, &info) != 0 && info.dli_fname != NULL) {
        if (NULL != info.dli_fbase && NULL != pc) {
            trackInfo->append(" ");
            trackInfo->append("pc");
            trackInfo->append(" ");
            //相对偏移地址
            const uintptr_t addr_relative =
                    (pc - (uintptr_t) info.dli_fbase);
            trackInfo->append(addr_relative);
        }

        if (NULL != info.dli_fname) {
            trackInfo->append(" ");
            trackInfo->append(info.dli_fname);
        }

        if (NULL != info.dli_sname) {
            trackInfo->append(" ");
            trackInfo->append("(");
            trackInfo->append(info.dli_sname);
            trackInfo->append("(");
            trackInfo->append(")");
        }

        if (NULL != info.dli_saddr) {
            trackInfo->append("+");
            const uintptr_t offset =
                    (pc - (uintptr_t) info.dli_saddr);
            trackInfo->append(offset);
            trackInfo->append(")");
        }
    } else {
        trackInfo->append("unKnow frame");
    }
    trackInfo->append("\n");
}

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