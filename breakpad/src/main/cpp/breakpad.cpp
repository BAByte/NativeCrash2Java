#include <jni.h>
#include <android/log.h>

#include "client/linux/handler/exception_handler.h"
#include "client/linux/handler/minidump_descriptor.h"
#include <pthread.h>
#include <semaphore.h>
#include <third_party/thread/java_thread.h>
#include <sys/system_properties.h>
#include <client/linux/log/log.h>
#include <client/linux/minidump_whole_writer/minidump_whole_track_writer.h>
#include <third_party/utils/crash_info_.h>

#define LOG_TAG ">>> breakpad_cpp"

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

JavaVM *g_jvm;
//当回调线程通知完java层后通知崩溃线程继续执行
static sem_t g_done_semaphore;
static sem_t g_cat_java_crash_semaphore;
jobject g_crash_callback_obj;
jmethodID g_crash_callback_method;

class CrashInfo {
public:
    char *thread;
    char *native_log;
    char *java_log;
    char *path;
};

volatile CrashInfo *g_crash_info = nullptr;

bool DumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
                  void *context,
                  bool succeeded) {
    ALOGD("===============crrrrash================");
    ALOGD("Dump path: %s\n", descriptor.path());
    jboolean needAttach = false;
    JNIEnv *env;
    jint res = g_jvm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (res == JNI_EDETACHED) {
        int result = g_jvm->AttachCurrentThread(&env, 0);
        if (result != 0) {
            return succeeded;
        }
        needAttach = true;
    }
    babyte::JavaThread javaThread = babyte::JavaThread(env);
    g_crash_info = new CrashInfo();
    g_crash_info->thread = javaThread.toString(env);
    g_crash_info->native_log = descriptor.minidump_whole_extra_info_.log_;
    g_crash_info->java_log = javaThread.getTrackInfo(env);
    g_crash_info->path = const_cast<char *>(descriptor.path());

    if (needAttach) {
        g_jvm->DetachCurrentThread();
    }

    sem_post(&g_cat_java_crash_semaphore);
    sem_wait(&g_done_semaphore);
    return succeeded;
}

/**
 * 当发生异常后，该线程会被执行
 */
void *ThreadHandler(void *argv) {
    JNIEnv *env;
    jboolean needAttach = false;
    jint res = g_jvm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (res == JNI_EDETACHED) {
        int result = g_jvm->AttachCurrentThread(&env, 0);
        if (result != 0) {
            return nullptr;
        }
        needAttach = true;
    }
    sem_wait(&g_cat_java_crash_semaphore);
    jstring info_js = nullptr;
    auto new_2line = "\n\n";
    char final_info[
            strlen(g_crash_info->thread) + strlen(g_crash_info->java_log) +
            strlen(g_crash_info->native_log) + strlen(new_2line)];
    if (g_crash_info != nullptr) {
        strcat(final_info, g_crash_info->native_log);
        strcat(final_info, new_2line);
        strcat(final_info, g_crash_info->thread);
        strcat(final_info, g_crash_info->java_log);
        delete g_crash_info;
        babyte::correctUtfBytes()
        info_js = env->NewStringUTF(final_info);
    }

    if (g_crash_callback_method != nullptr) {
        if (info_js == nullptr) {
            auto error = "get crash error";
            jstring error_js = env->NewStringUTF(error);
            info_js = error_js;
        }
        env->CallVoidMethod(g_crash_callback_obj, g_crash_callback_method,
                            info_js);
    } else {
        ALOGE("threadHandler g_crash_callback_method == null");
    }
    sem_post(&g_done_semaphore);
    if (needAttach) {
        g_jvm->DetachCurrentThread();
    }
    return nullptr;
}

static void initCallbackMethod(JNIEnv *env, jobject callback) {
    g_crash_callback_obj = env->NewGlobalRef(callback);
    jclass cls = env->GetObjectClass(g_crash_callback_obj);
    if (cls == NULL) {
        ALOGE("ThreadHandler get class error");
        return;
    }
    g_crash_callback_method = env->GetMethodID(cls, "onCrash",
                                               "(Ljava/lang/String;)V");
    if (g_crash_callback_method == NULL) {
        ALOGE("ThreadHandler get jmethodId error");
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_babyte_breakpad_BaByteBreakpad_initBreakpadNative(JNIEnv *env, jobject thiz, jstring path_,
                                                           jobject callback) {
    initCallbackMethod(env, callback);

    const char *path = env->GetStringUTFChars(path_, 0);
    google_breakpad::MinidumpDescriptor descriptor(path);
    descriptor.minidump_whole_extra_info()->g_jvm = g_jvm;

    static google_breakpad::ExceptionHandler eh(descriptor, nullptr, DumpCallback, NULL,
                                                true, -1);
    env->ReleaseStringUTFChars(path_, path);
}

static void initCallbackThread() {
    sem_init(&g_cat_java_crash_semaphore, 0, 0);
    sem_init(&g_done_semaphore, 0, 0);
    //崩溃时开另一个线程回调上层java方法
    pthread_t thd;
    int ret = pthread_create(&thd, NULL, ThreadHandler, NULL);
    if (ret) {
        ALOGE("%s", "pthread_create error");
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    initCallbackThread();
    JNIEnv *env;
    g_jvm = vm;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_6;
}