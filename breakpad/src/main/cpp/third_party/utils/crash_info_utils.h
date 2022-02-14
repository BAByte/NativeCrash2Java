//
// Created by ba on 1/25/22.
//

#ifndef CRASH_INFO_UTILS_H
#define CRASH_INFO_UTILS_H

#include <stddef.h>
#include <client/linux/minidump_writer/linux_dumper.h>
#include <dlfcn.h>
#include "common/memory_allocator.h"

namespace babyte {
    class CrashInfo {
    public:
        size_t kLineBufferSize = 4 * 1024;
        char *log_line_;
        google_breakpad::LinuxDumper *dumper_;

        CrashInfo(google_breakpad::LinuxDumper *dumper, size_t size);

        ~CrashInfo();

        void *Alloc(unsigned bytes) { return dumper_->allocator()->Alloc(bytes); }


        void append(const char *str);

        void append(char *str);

        void append(int i);

        void append(long i);

        // Stages the hex repr. of the given int type in the current line buffer.
        template<typename T>
        void append(T value) {
            // Make enough room to hex encode the largest int type + NUL.
            static const char HEX[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                       'A', 'B', 'C', 'D', 'E', 'F'};
            char hexstr[sizeof(T) * 2 + 1];
            for (int i = sizeof(T) * 2 - 1; i >= 0; --i, value >>= 4)
                hexstr[i] = HEX[static_cast<uint8_t>(value) & 0x0F];
            hexstr[sizeof(T) * 2] = '\0';
            append(hexstr);
        }

        void append(const void *buf, size_t length);

        void format(uintptr_t pc, babyte::CrashInfo *trackInfo, int line);

        void clear();
    };
}
#endif //CRASH_INFO_UTILS_H
