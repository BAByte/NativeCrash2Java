//
// Created by ba on 2/10/22.
//

#ifndef BANATIVECRASH_LIBBACKTRACE_UTILS_H
#define BANATIVECRASH_LIBBACKTRACE_UTILS_H

#include <ucontext.h>
#include <third_party/utils/crash_info_utils.h>
#include "backtrace.h"

namespace babyte {
    class LibBackTraceUtils {
    public:
        const char *libBacktraceName = "libbacktrace.so";
        const char *createBacktraceName = "_ZN9Backtrace6CreateEiiP12BacktraceMap";
        void dumpTrace(pid_t tid, void* context, babyte::CrashInfo *trackInfo);
        BacktraceStub* getBacktrace(pid_t tid);
        void release();
    };
}

#endif //BANATIVECRASH_LIBBACKTRACE_UTILS_H
