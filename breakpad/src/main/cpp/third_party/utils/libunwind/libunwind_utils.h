//
// Created by ba on 1/25/22.
//

#ifndef LIBUNWIND_UTILS_H
#define LIBUNWIND_UTILS_H

#include <client/linux/minidump_writer/linux_dumper.h>
#include "third_party/utils/crash_info_utils.h"

namespace babyte {
    class LibunwindUtils {
    public:
        const char* libunwindName = "libunwind.so";
        bool initLibunwind();

        void dumpTrack(ucontext_t *uc, int limit, babyte::CrashInfo *trackInfo);

        void release();
    };
}

#endif //LIBUNWIND_UTILS_H
