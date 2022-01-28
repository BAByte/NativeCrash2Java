//
// Created by ba on 1/25/22.
//

#ifndef CRASH_INFO__H
#define CRASH_INFO__H

#include <stddef.h>
#include <client/linux/minidump_writer/linux_dumper.h>
#include "common/memory_allocator.h"

namespace babyte {
    class CrashInfo {
    public:
        size_t kLineBufferSize = 8 * 1024;
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

        void clear();

        int correctUtfBytes(char *bytes) {
            char three = 0;
            while (*bytes != '\0') {
                unsigned char utf8 = *(bytes++);
                three = 0;
                // Switch on the high four bits.
                switch (utf8 >> 4) {
                    case 0x00:
                    case 0x01:
                    case 0x02:
                    case 0x03:
                    case 0x04:
                    case 0x05:
                    case 0x06:
                    case 0x07:
                        // Bit pattern 0xxx. No need for any extra bytes.
                        break;
                    case 0x08:
                    case 0x09:
                    case 0x0a:
                    case 0x0b:
                    case 0x0f:
                        /*
                         * Bit pattern 10xx or 1111, which are illegal start bytes.
                         * Note: 1111 is valid for normal UTF-8, but not the
                         * modified UTF-8 used here.
                         */
                        *(bytes - 1) = '?';
                        break;
                    case 0x0e:
                        // Bit pattern 1110, so there are two additional bytes.
                        utf8 = *(bytes++);
                        if ((utf8 & 0xc0) != 0x80) {
                            --bytes;
                            *(bytes - 1) = '?';
                            break;
                        }
                        three = 1;
                        // Fall through to take care of the final byte.
                    case 0x0c:
                    case 0x0d:
                        // Bit pattern 110x, so there is one additional byte.
                        utf8 = *(bytes++);
                        if ((utf8 & 0xc0) != 0x80) {
                            --bytes;
                            if (three)--bytes;
                            *(bytes - 1) = '?';
                        }
                        break;
                }
            }
        }
    };
}
#endif //CRASH_INFO__H
