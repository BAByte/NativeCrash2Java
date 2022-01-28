//
// Created by ba on 1/21/22.
//

#ifndef MINIDUMP_WHOLE_TRACK_EXTRA_INFO_H
#define MINIDUMP_WHOLE_TRACK_EXTRA_INFO_H
#include <jni.h>

namespace babyte {

    struct MinidumpWholeExtraInfo {
        // Strings pointed to by this struct are not copied, and are
        // expected to remain valid for the lifetime of the process.
        const char* build_fingerprint;
        const char* product_info;
        const char* gpu_fingerprint;
        const char* process_type;
        char *log_;
        JavaVM *g_jvm;

        MinidumpWholeExtraInfo()
                : build_fingerprint(NULL),
                  product_info(NULL),
                  gpu_fingerprint(NULL),
                  process_type(NULL) {}
    };
}
#endif //MINIDUMP_WHOLE_TRACK_EXTRA_INFO_H
