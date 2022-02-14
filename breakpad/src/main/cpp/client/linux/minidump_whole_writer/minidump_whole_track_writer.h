//
// Created by ba on 1/21/22.
//

#ifndef MINIDUMP_WHOLE_TRACK_WRITER_H
#define MINIDUMP_WHOLE_TRACK_WRITER_H

#include <stdint.h>
#include <sys/types.h>
#include <client/linux/dump_writer_common/mapping_info.h>

namespace babyte {

// Writes a microdump (a reduced dump containing only the state of the crashing
// third_party.thread) on the console (logcat on Android). These functions do not malloc nor
// use libc functions which may. Thus, it can be used in contexts where the
// state of the heap may be corrupt.
// Args:
//   crashing_process: the pid of the crashing process. This must be trusted.
//   blob: a blob of data from the crashing process. See exception_handler.h
//   blob_size: the length of |blob| in bytes.
//   mappings: a list of additional mappings provided by the application.
//   build_fingerprint: a (optional) C string which determines the OS
//     build fingerprint (e.g., aosp/occam/mako:5.1.1/LMY47W/1234:eng/dev-keys).
//   product_info: a (optional) C string which determines the product name and
//     version (e.g., WebView:42.0.2311.136).
//
// Returns true iff successful.
    bool WriteWholeMinidump(pid_t crashing_process, const void *blob, size_t blob_size,
                            int log_descriptors[2]);

    static const char *HEAD_THREAD = "^";
    const int MAX_LOG_NUM_ = 10 * 1024;
    const int MAX_FRAME_ = 64;
    const int ANDROID_L = 21;
    const int ANDROID_N = 24;
    const int PIPELINE_WRITE = 1;
    const int PIPELINE_READ = 0;
}  // namespace babyte

#endif //MINIDUMP_WHOLE_TRACK_WRITER_H
