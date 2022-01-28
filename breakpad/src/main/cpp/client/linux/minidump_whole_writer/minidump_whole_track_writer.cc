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
#include <third_party/thread/java_thread.h>

#include "client/linux/dump_writer_common/thread_info.h"
#include "client/linux/dump_writer_common/ucontext_reader.h"
#include "client/linux/handler/exception_handler.h"
#include "client/linux/log/log.h"
#include "client/linux/minidump_writer/linux_ptrace_dumper.h"
#include "common/linux/file_id.h"
#include "common/linux/linux_libc_support.h"
#include "common/memory_allocator.h"
#include "client/linux/minidump_whole_writer/minidump_whole_track_writer.h"
#include <google_breakpad/common/minidump_exception_linux.h>
#include <dlfcn.h>
#include <third_party/utils/libunwind_utils.h>
#include <third_party/utils/crash_info_.h>
#include <third_party/utils/android_version_utils.h>
#include <linux/prctl.h>
#include <sys/prctl.h>

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
        const MappingList &mapping_list_;
        bool skip_dump_if_principal_mapping_not_referenced_;
        uintptr_t address_within_principal_mapping_;
        bool sanitize_stack_;
        babyte::MinidumpWholeExtraInfo minidump_whole_extra_info_;
        babyte::CrashInfo stringSplicing_;
    public:
        const char *GetCrashSignalCodeString(int crash_signal_, int crash_signal_code) const {
            switch (static_cast<unsigned int>(crash_signal_)) {
                case SIGBUS:
                    return GetSIGBUS(crash_signal_code);
                case SIGFPE:
                    return GetSIGFPE(crash_signal_code);
                case SIGSEGV:
                    return GetSIGSEGV(crash_signal_code);
                case SIGILL:
                    return GetSIGILL(crash_signal_code);
                default:
                    return reinterpret_cast<const char *>(crash_signal_code);
            }
        }

        const char *GetSIGBUS(int crash_signal_code) const {
            switch (static_cast<unsigned int>(crash_signal_code)) {
                case MD_EXCEPTION_FLAG_LIN_BUS_ADRALN:
                    return "address cannot be aligned";
                case MD_EXCEPTION_FLAG_LIN_BUS_ADRERR:
                    return "wrong physical address";
                default :
                    return reinterpret_cast<const char *>(crash_signal_code);
            }
        }

        const char *GetSIGFPE(int crash_signal_code) const {
            switch (static_cast<unsigned int>(crash_signal_code)) {
                case MD_EXCEPTION_FLAG_LIN_FPE_INTDIV:
                    return "int / 0";
                case MD_EXCEPTION_FLAG_LIN_FPE_INTOVF:
                    return "int overflow";
                case MD_EXCEPTION_FLAG_LIN_FPE_FLTDIV:
                    return "float / 0";
                case MD_EXCEPTION_FLAG_LIN_FPE_FLTOVF:
                    return "float overflow";
                case MD_EXCEPTION_FLAG_LIN_FPE_FLTUND:
                    return "floating point underflow";
                case MD_EXCEPTION_FLAG_LIN_FPE_FLTRES:
                    return "floating point numbers are imprecise";
                case MD_EXCEPTION_FLAG_LIN_FPE_FLTINV:
                    return "illegal floating point operation";
                case MD_EXCEPTION_FLAG_LIN_FPE_FLTSUB:
                    return "The table below is out of bounds";
                default :
                    return reinterpret_cast<const char *>(crash_signal_code);
            }
        }

        const char *GetSIGSEGV(int crash_signal_code) const {
            switch (static_cast<unsigned int>(crash_signal_code)) {
                case MD_EXCEPTION_FLAG_LIN_SEGV_MAPERR:
                    return "Invalid address";
                case MD_EXCEPTION_FLAG_LIN_SEGV_ACCERR:
                    return "address without access";
                default :
                    return reinterpret_cast<const char *>(crash_signal_code);
            }
        }

        const char *GetSIGILL(int crash_signal_code) const {
            switch (static_cast<unsigned int>(crash_signal_code)) {
                case MD_EXCEPTION_FLAG_LIN_ILL_ILLOPC:
                    return "Illegal opcode";
                case MD_EXCEPTION_FLAG_LIN_ILL_ILLOPN:
                    return "Illegal operand";
                case MD_EXCEPTION_FLAG_LIN_ILL_ILLADR:
                    return "Illegal addressing mode";
                case MD_EXCEPTION_FLAG_LIN_ILL_ILLTRP:
                    return "Illegal trap";
                case MD_EXCEPTION_FLAG_LIN_ILL_PRVOPC:
                    return "Privileged opcode";
                case MD_EXCEPTION_FLAG_LIN_ILL_PRVREG:
                    return "Privileged register";
                case MD_EXCEPTION_FLAG_LIN_ILL_COPROC:
                    return "Coprocessor error";
                case MD_EXCEPTION_FLAG_LIN_ILL_BADSTK:
                    return "Internal stack error";
                default :
                    return reinterpret_cast<const char *>(crash_signal_code);
            }
        }

        MinidumpWholeWriter(const ExceptionHandler::CrashContext *context,
                            const MappingList &mappings,
                            bool skip_dump_if_principal_mapping_not_referenced,
                            uintptr_t address_within_principal_mapping,
                            bool sanitize_stack,
                            const babyte::MinidumpWholeExtraInfo &minidump_whole_extra_info,
                            LinuxDumper *dumper)
                : ucontext_(context ? &context->context : NULL),
#if !defined(__ARM_EABI__) && !defined(__mips__)
                  float_state_(context ? &context->float_state : NULL),
#endif
                  dumper_(dumper),
                  mapping_list_(mappings),
                  skip_dump_if_principal_mapping_not_referenced_(
                          skip_dump_if_principal_mapping_not_referenced),
                  address_within_principal_mapping_(address_within_principal_mapping),
                  sanitize_stack_(sanitize_stack),
                  minidump_whole_extra_info_(minidump_whole_extra_info),
                  stringSplicing_(babyte::CrashInfo(dumper, babyte::MAX_LOG_NUM_)) {}

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
            stringSplicing_.append("------------------- NATIVE: CRASH INFO IN LIBRARY:\n");
            DumpProductInformation();
            DumpOSInformation();
            stringSplicing_.append("\n");
            DumpCpuInfo();
            stringSplicing_.append("\n");
            stringSplicing_.append("\n");
            DumpCrashReason();
            stringSplicing_.append("\n");
            DumpCrashInfoInLib();
            stringSplicing_.append("\n");
            stringSplicing_.append("\n");
            stringSplicing_.append("------------------- CRASH THREAD TRACK:\n");
            DumpNativeThreadTrash();
            minidump_whole_extra_info_.log_[0] = '\0';
            my_strlcpy(minidump_whole_extra_info_.log_, stringSplicing_.log_line_,
                       strlen(stringSplicing_.log_line_));
            stringSplicing_.clear();
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
#if defined(__ANDROID__)
            const char kOSId[] = "A";
#else
            const char kOSId[] = "L";
#endif

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
                stringSplicing_.append("CrashSo: ");
                stringSplicing_.append(info.dli_fname);
                stringSplicing_.append("(");
                stringSplicing_.append(kArch);
                stringSplicing_.append(")");
                stringSplicing_.append(" ");
                stringSplicing_.append("+");
                stringSplicing_.append((uintptr_t) addr_relative);
                stringSplicing_.append("\n");

                stringSplicing_.append("CrashMethod: ");
                stringSplicing_.append((char *) info.dli_sname);
                stringSplicing_.append("\n");
                stringSplicing_.append(
                        "NOTE: \n   You can use the add2line tool and the library with symbol to get the wrong line of code. For example:\n   $ ./xxxx-add2line -Cfe xxxx.so ");
                stringSplicing_.append((uintptr_t) addr_relative);
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
            stringSplicing_.append(GetCrashSignalCodeString(
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
            stringSplicing_.append(",tid:");
            stringSplicing_.append(syscall(SYS_gettid));
            stringSplicing_.append("]");
            stringSplicing_.append("\n");

            int versionCode = babyte::getSDKVersion();
            if (versionCode >= babyte::ANDROID_L && versionCode < babyte::ANDROID_N) {
                DumpNativeThreadTrashBeforeN();
                return;
            }
            if (versionCode >= babyte::ANDROID_N) {
                DumpNativeThreadTrashAfterN();
            }
        }

        void DumpNativeThreadTrashBeforeN() {
            babyte::LibunwindUtils libunwindUtils = babyte::LibunwindUtils();
            if (libunwindUtils.initLibunwind()) {
                libunwindUtils.dumpTrack(const_cast<ucontext_t *>(ucontext_),
                                         babyte::MAX_FRAME_, &stringSplicing_);
                libunwindUtils.release();
            }
        }

        void DumpNativeThreadTrashAfterN() {

        }
    };
}


namespace babyte {

    bool WriteWholeMinidump(pid_t crashing_process,
                            const void *blob,
                            size_t blob_size,
                            const MappingList &mappings,
                            bool skip_dump_if_principal_mapping_not_referenced,
                            uintptr_t address_within_principal_mapping,
                            bool sanitize_stack,
                            const babyte::MinidumpWholeExtraInfo &minidump_whole_extra_info) {
        LinuxPtraceDumper dumper(crashing_process);
        const ExceptionHandler::CrashContext *context = NULL;
        if (blob) {
            if (blob_size != sizeof(ExceptionHandler::CrashContext))
                return false;
            context = reinterpret_cast<const ExceptionHandler::CrashContext *>(blob);
            dumper.SetCrashInfoFromSigInfo(context->siginfo);
            dumper.set_crash_thread(context->tid);
        }
        MinidumpWholeWriter writer(context, mappings,
                                   skip_dump_if_principal_mapping_not_referenced,
                                   address_within_principal_mapping, sanitize_stack,
                                   minidump_whole_extra_info, &dumper);
        if (!writer.Init())
            return false;
        writer.Dump();
        return true;
    }

}  // namespace babyte

