//
// Created by ba on 1/25/22.
//

#include <dlfcn.h>
#include <ucontext.h>
#include "libunwind_utils.h"
#include <android/log.h>
#include <cstdlib>
#include <cstring>

#define LOG_TAG ">>> Libunwind"
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#if defined(__arm__)
#define UNW_TARGET "arm"
#define UNW_REG_IP 14
#define UNW_CURSOR_LEN 4096
#elif defined(__aarch64__)
#define UNW_TARGET "aarch64"
#define UNW_REG_IP 30
#define UNW_CURSOR_LEN 4096
#elif defined(__i386__)
#define UNW_TARGET "x86"
#define UNW_REG_IP 8
#define UNW_CURSOR_LEN 127
#elif defined(__x86_64__)
#define UNW_TARGET "x86_64"
#define UNW_REG_IP 16
#define UNW_CURSOR_LEN 127
#endif

typedef struct {
    uintptr_t opaque[UNW_CURSOR_LEN];
} unw_cursor_t;

#if defined(__arm__)
typedef struct {
    uintptr_t r[16];
} unw_context_t;
#else
typedef ucontext_t unw_context_t;
#endif

typedef int (*t_unw_init_local)(unw_cursor_t *, unw_context_t *);

typedef int (*t_unw_get_reg)(unw_cursor_t *, int, uintptr_t *);

typedef int (*t_unw_step)(unw_cursor_t *);

static unw_cursor_t *cursor = nullptr;
static unw_context_t *context = nullptr;
static t_unw_init_local unw_init_local = nullptr;
static t_unw_get_reg unw_get_reg = nullptr;
static t_unw_step unw_step = nullptr;
void *libunwind = nullptr;
size_t kLineBufferSize = 8 * 1024;

bool babyte::LibunwindUtils::initLibunwind() {
    if (nullptr == (libunwind = dlopen(libunwindName, RTLD_NOW))) {
        ALOGE("dlopen libunwind = null");
        release();
        return false;
    }

    if (nullptr ==
        (unw_init_local = (t_unw_init_local) dlsym(libunwind, "_U" UNW_TARGET"_init_local"))) {
        ALOGE("dlsym unw_init_local = null");
        release();
        return false;
    }

    if (nullptr == (unw_get_reg = (t_unw_get_reg) dlsym(libunwind, "_U" UNW_TARGET"_get_reg"))) {
        ALOGE("dlsym unw_get_reg = null");
        release();
        return false;
    }

    if (nullptr == (unw_step = (t_unw_step) dlsym(libunwind, "_U" UNW_TARGET"_step"))) {
        ALOGE("dlsym unw_step = null");
        release();
        return false;
    }
    if (nullptr == (cursor = static_cast<unw_cursor_t *>(calloc(1, sizeof(unw_cursor_t))))) {
        ALOGE("calloc cursor = null");
        release();
        return false;
    }
    if (nullptr == (context = static_cast<unw_context_t *>(calloc(1, sizeof(unw_context_t))))) {
        ALOGE("calloc context = null");
        release();
        return false;
    }
    return true;
}

void babyte::LibunwindUtils::dumpTrack(ucontext_t *uc, int limit, babyte::CrashInfo *trackInfo) {
#if defined(__arm__)
    context->r[0] = uc->uc_mcontext.arm_r0;
    context->r[1] = uc->uc_mcontext.arm_r1;
    context->r[2] = uc->uc_mcontext.arm_r2;
    context->r[3] = uc->uc_mcontext.arm_r3;
    context->r[4] = uc->uc_mcontext.arm_r4;
    context->r[5] = uc->uc_mcontext.arm_r5;
    context->r[6] = uc->uc_mcontext.arm_r6;
    context->r[7] = uc->uc_mcontext.arm_r7;
    context->r[8] = uc->uc_mcontext.arm_r8;
    context->r[9] = uc->uc_mcontext.arm_r9;
    context->r[10] = uc->uc_mcontext.arm_r10;
    context->r[11] = uc->uc_mcontext.arm_fp;
    context->r[12] = uc->uc_mcontext.arm_ip;
    context->r[13] = uc->uc_mcontext.arm_sp;
    context->r[14] = uc->uc_mcontext.arm_lr;
    context->r[15] = uc->uc_mcontext.arm_pc;
#else
    memcpy(context, uc, sizeof(ucontext_t));
#endif
    if (unw_init_local(cursor, context) < 0) {
        ALOGE("t_unw_init_local error");
        return;
    }
    int i = 0;
    do {
        uintptr_t pc;
        if (unw_get_reg(cursor, UNW_REG_IP, &pc) < 0) {
            trackInfo->append("unKnow frame");
            continue;
        }
        trackInfo->format(pc,trackInfo, i);
        i++;
    } while (unw_step(cursor) > 0 && i < limit);
}

void babyte::LibunwindUtils::release() {
    if (libunwind != nullptr) {
        free(cursor);
        free(context);
        cursor = nullptr;
        context = nullptr;
        unw_init_local = nullptr;
        unw_get_reg = nullptr;
        unw_step = nullptr;
        dlclose(libunwind);
        libunwind = nullptr;
    }
}