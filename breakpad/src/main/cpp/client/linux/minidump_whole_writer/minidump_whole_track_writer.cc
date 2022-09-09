// Copyright (c) 2014, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This translation unit generates microdumps into the console (logcat on
// Android). See crbug.com/410294 for more info and design docs.


#include <limits>

#include <sys/utsname.h>
#include <android/log.h>

#include "client/linux/dump_writer_common/thread_info.h"
#include "client/linux/dump_writer_common/ucontext_reader.h"
#include "client/linux/handler/exception_handler.h"
#include "client/linux/log/log.h"
#include "client/linux/minidump_writer/linux_ptrace_dumper.h"
#include "common/linux/linux_libc_support.h"
#include "client/linux/minidump_whole_writer/minidump_whole_track_writer.h"
#include <dlfcn.h>
#include <sys/prctl.h>
#include <third_party/utils/libunwind/libunwind_utils.h>
#include <third_party/utils/crash_info_utils.h>
#include <third_party/utils/android_version_utils.h>
#include <third_party/utils/signal_explain.h>
#include <third_party/utils/libbacktrace/libbacktrace_utils.h>

#define LOG_TAG ">>> minidumpWholeTrackWrier"

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
    using google_breakpad::auto_wasteful_vector;
    using google_breakpad::ExceptionHandler;
    using google_breakpad::kDefaultBuildIdSize;
    using google_breakpad::LinuxDumper;
    using google_breakpad::LinuxPtraceDumper;
    using google_breakpad::MappingInfo;
    using google_breakpad::MappingList;
    using google_breakpad::MicrodumpExtraInfo;
    using google_breakpad::RawContextCPU;
    using google_breakpad::ThreadInfo;
    using google_breakpad::UContextReader;

    class MinidumpWholeWriter {
    private:
        const ucontext_t *const ucontext_;
#if !defined(__ARM_EABI__) && !defined(__mips__)
        const google_breakpad::fpstate_t *const float_state_;
#endif
        LinuxDumper *dumper_;
        babyte::CrashInfo stringSplicing_;
        int *log_descriptors_;
    public:
        MinidumpWholeWriter(const ExceptionHandler::CrashContext *context,
                            LinuxDumper *dumper,
                            int log_descriptors[2])
                : ucontext_(context ? &context->context : NULL),
#if !defined(__ARM_EABI__) && !defined(__mips__)
                float_state_(context ? &context->float_state : NULL),
#endif
                  dumper_(dumper),
                  stringSplicing_(babyte::CrashInfo(dumper, babyte::MAX_LOG_NUM_)),
                  log_descriptors_(log_descriptors) {}

        ~MinidumpWholeWriter() { dumper_->ThreadsResume(); }

        bool Init() {
            // In the exceptional case where the system was out of memory and there
            // wasn't even room to allocate the line buffer, bail out. There is nothing
            // useful we can possibly achieve without the ability to Log. At least let's
            // try to not crash.
            if (!dumper_->Init())
                return false;
            return dumper_->ThreadsSuspend() && dumper_->LateInit();
        }

        void Dump() {
            ALOGD("WriteWholeMinidump Dump");
            DumpProductInformation();
            DumpOSInformation();
            stringSplicing_.append("\n");
            DumpCpuInfo();
            stringSplicing_.append("\n");
            stringSplicing_.append("\n");
            DumpCrashReason();
            stringSplicing_.append("\n");
            DumpCrashInfoInLib();
            stringSplicing_.append(babyte::HEAD_THREAD);
            DumpNativeThreadTrash();
            close(log_descriptors_[babyte::PIPELINE_READ]);
            write(log_descriptors_[babyte::PIPELINE_WRITE],
                  stringSplicing_.log_line_,
                  strlen(stringSplicing_.log_line_));
            close(log_descriptors_[1]);
            stringSplicing_.clear();
            ALOGD("WriteWholeMinidump Dump end");
        }

    private:
        void DumpProductInformation() {
            stringSplicing_.append("Operating system: Android ");
            stringSplicing_.append(babyte::getSDKVersion());
        }

        void DumpOSInformation() {
            // Dump the HW architecture (e.g., armv7l, aarch64).
            struct utsname uts;
            const bool has_uts_info = (uname(&uts) == 0);
            if (has_uts_info) {
                stringSplicing_.append(" ");
                stringSplicing_.append(uts.sysname);
                stringSplicing_.append(" ");
                stringSplicing_.append(uts.release);
                stringSplicing_.append(" ");
                stringSplicing_.append(uts.version);
            }
        }

        void DumpCpuInfo() {
            const int n_cpus = sysconf(_SC_NPROCESSORS_CONF);
            struct utsname uts;
            const bool has_uts_info = (uname(&uts) == 0);
            const char *hwArch = has_uts_info ? uts.machine : "unknown_hw_arch";
            stringSplicing_.append("CPU:");
            stringSplicing_.append(" ");
            stringSplicing_.append(hwArch);
            stringSplicing_.append(" ");
            stringSplicing_.append("(");
            stringSplicing_.append(n_cpus);
            stringSplicing_.append(" ");
            stringSplicing_.append("core");
            stringSplicing_.append(")");
        }

        void DumpCrashInfoInLib() {
// Dump the runtime architecture. On multiarch devices it might not match the
// hw architecture (the one returned by uname()), for instance in the case of
// a 32-bit app running on a aarch64 device.
#if defined(__aarch64__)
            const char kArch[] = "arm64";
#elif defined(__ARMEL__)
            const char kArch[] = "arm";
#elif defined(__x86_64__)
            const char kArch[] = "x86_64";
#elif defined(__i386__)
            const char kArch[] = "x86";
#elif defined(__mips__)
# if _MIPS_SIM == _ABIO32
            const char kArch[] = "mips";
# elif _MIPS_SIM == _ABI64
            const char kArch[] = "mips64";
# else
#  error "This mips ABI is currently not supported (n32)"
#endif
#else
#error "This code has not been ported to your platform yet"
#endif
            uintptr_t pc = UContextReader::GetInstructionPointer(ucontext_);
            Dl_info info;
            if (dladdr((void *) pc, &info) != 0 && info.dli_fname != NULL) {
                //相对偏移地址
                const uintptr_t addr_relative =
                        (pc - (uintptr_t) info.dli_fbase);
                stringSplicing_.append("Crash pc:");
                stringSplicing_.append(" ");
                stringSplicing_.append((uintptr_t) addr_relative);
                stringSplicing_.append("\n");
                stringSplicing_.append("Crash so:");
                stringSplicing_.append(" ");
                stringSplicing_.append(info.dli_fname);
                stringSplicing_.append("(");
                stringSplicing_.append(kArch);
                stringSplicing_.append(")");
                stringSplicing_.append("\n");

                stringSplicing_.append("Crash method: ");
                stringSplicing_.append((char *) info.dli_sname);
            } else {
                stringSplicing_.append("Microdump skipped (uninteresting)");
            }
        }

        void DumpCrashReason() {
            stringSplicing_.append("Crash reason: signal ");
            stringSplicing_.append(dumper_->crash_signal());
            stringSplicing_.append("(");
            stringSplicing_.append(dumper_->GetCrashSignalString());
            stringSplicing_.append(")");
            stringSplicing_.append(" ");
            stringSplicing_.append(babyte::GetCrashSignalCodeString(
                    dumper_->crash_signal(),
                    dumper_->crash_signal_code()));
            stringSplicing_.append("\n");
            stringSplicing_.append("Crash address: ");
            stringSplicing_.append(dumper_->crash_address());
        }

        void DumpNativeThreadTrash() {
            char cThreadName[32] = {0};
            prctl(PR_GET_NAME, (unsigned long) cThreadName);
            stringSplicing_.append("Thread[name:");
            stringSplicing_.append(cThreadName);
            stringSplicing_.append("]");
            stringSplicing_.append(" (NOTE: linux thread name length limit is 15 characters)");
            stringSplicing_.append("\n");
            int versionCode = babyte::getSDKVersion();
            if (versionCode >= babyte::ANDROID_L && versionCode < babyte::ANDROID_O) {
                DumpNativeThreadTrashBeforeO();
            } else if (versionCode >= babyte::ANDROID_O) {
                DumpNativeThreadTrashAfterO();
            }
        }

        void DumpNativeThreadTrashBeforeO() {
            babyte::LibunwindUtils libunwindUtils = babyte::LibunwindUtils();
            if (libunwindUtils.initLibunwind()) {
                libunwindUtils.dumpTrack(const_cast<ucontext_t *>(ucontext_),
                                         babyte::MAX_FRAME_, &stringSplicing_);
                libunwindUtils.release();
            }
        }

        void DumpNativeThreadTrashAfterO() {
            babyte::LibBackTraceUtils libBackTraceUtils = babyte::LibBackTraceUtils();
            libBackTraceUtils.dumpTrace(dumper_->crash_thread(), (void *) ucontext_,
                                        &stringSplicing_);
        }
    };
}


namespace babyte {

    bool WriteWholeMinidump(pid_t crashing_process,
                            const void *blob,
                            size_t blob_size, int log_descriptors[2]) {
        ALOGD("WriteWholeMinidump");
        LinuxPtraceDumper dumper(crashing_process);
        const ExceptionHandler::CrashContext *context = NULL;
        if (blob) {
            if (blob_size != sizeof(ExceptionHandler::CrashContext))
                return false;
            context = reinterpret_cast<const ExceptionHandler::CrashContext *>(blob);
            dumper.SetCrashInfoFromSigInfo(context->siginfo);
            dumper.set_crash_thread(context->tid);
        }
        MinidumpWholeWriter writer(context, &dumper,
                                   log_descriptors);
        if (!writer.Init())
            return false;
        writer.Dump();
        return true;
    }

}  // namespace babyte

