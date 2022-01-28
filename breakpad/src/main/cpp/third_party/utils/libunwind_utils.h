//
// Created by ba on 1/25/22.
//

#ifndef LIBUNWIND_UTILS_H
#define LIBUNWIND_UTILS_H

#include <client/linux/minidump_writer/linux_dumper.h>
#include "crash_info_.h"

namespace babyte {
    class LibunwindUtils {
    public:
        bool initLibunwind();

        void dumpTrack(ucontext_t *uc, int limit, babyte::CrashInfo *trackInfo);

        void format(babyte::CrashInfo *trackInfo,int line);

        void release();
    };
}

#endif //LIBUNWIND_UTILS_H
