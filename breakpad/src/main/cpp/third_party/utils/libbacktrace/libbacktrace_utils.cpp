//
// Created by ba on 2/10/22.
//

#include <third_party/utils/ndk_dlopen/dlopen.h>
#include "libbacktrace_utils.h"
#include "backtrace.h"
#include <android/log.h>
#include <client/linux/dump_writer_common/ucontext_reader.h>
#include "backtrace_constants.h"

#define LOG_TAG ">>> LibBackTraceUtils"
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void *libbacktrace= nullptr;
typedef BacktraceStub* (*backtrace_create_t)(pid_t pid, pid_t tid,void *);
static backtrace_create_t backtrace_create_ = nullptr;
void
babyte::LibBackTraceUtils::dumpTrace(pid_t tid, void *context, babyte::CrashInfo *trackInfo) {
    std::unique_ptr<BacktraceStub> backtrace{getBacktrace(tid)};
    if (!backtrace || !backtrace->Unwind(0, context)) {
        return;
    }

    for (size_t i = 0, size = backtrace->NumFrames(); i < size; ++i) {
        auto str = backtrace->FormatFrameData(i);
        if (str.empty()) continue;
        trackInfo->append(str.c_str());
        trackInfo->append("\n");
    }
}
BacktraceStub *babyte::LibBackTraceUtils::getBacktrace(pid_t tid) {
    void* handle = ndk_dlopen(libBacktraceName, RTLD_NOW);
    if (!handle) {
        return nullptr;
    }

    backtrace_create_= (backtrace_create_t)ndk_dlsym(handle, createBacktraceName);
    if (!backtrace_create_) {
        return nullptr;
    }
    return backtrace_create_(BACKTRACE_CURRENT_PROCESS, tid, nullptr);
}

void babyte::LibBackTraceUtils::release() {
    if (libbacktrace != nullptr) {
        dlclose(libbacktrace);
        libbacktrace = nullptr;
    }
}