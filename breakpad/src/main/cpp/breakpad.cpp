#include <jni.h>
#include <android/log.h>

#include "client/linux/handler/exception_handler.h"
#include "client/linux/handler/minidump_descriptor.h"
#include <pthread.h>
#include <semaphore.h>
#include <sys/system_properties.h>
#include <client/linux/log/log.h>
#include <linux/prctl.h>
#include <sys/prctl.h>
#include <third_party/utils/ndk_dlopen/dlopen.h>
#include <third_party/utils/android_version_utils.h>
#include <client/linux/minidump_whole_writer/minidump_whole_track_writer.h>

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
    char *native_log;
    char *path;
    char *crash_thread_name;
};

volatile CrashInfo *g_crash_info = nullptr;

bool DumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
                  void *context,
                  bool succeeded,
                  char *log) {
    char cThreadName[32] = {0};
    prctl(PR_GET_NAME, (unsigned long) cThreadName);

    g_crash_info = new CrashInfo();
    g_crash_info->native_log = log;
    g_crash_info->path = const_cast<char *>(descriptor.path());
    g_crash_info->crash_thread_name = cThreadName;

    sem_post(&g_cat_java_crash_semaphore);
    sem_wait(&g_done_semaphore);
    return succeeded;
}

void notify2Java(JNIEnv *env) {
    jstring path_js = nullptr;
    auto error_path = "not dump path";
    jstring error_path_js = env->NewStringUTF(error_path);
    jstring info_js = nullptr;
    auto error = "get crash error";
    jstring error_js = env->NewStringUTF(error);

    if (g_crash_info != nullptr) {
        char final_info[strlen(g_crash_info->native_log)];
        strcat(final_info, g_crash_info->native_log);

        jbyteArray array = env->NewByteArray(strlen(final_info));
        env->SetByteArrayRegion(array, 0, strlen(final_info),
                                reinterpret_cast<const jbyte *>(final_info));
        jstring strEncode = env->NewStringUTF("UTF-8");
        jclass cls = env->FindClass("java/lang/String");
        jmethodID ctor = env->GetMethodID(cls, "<init>", "([BLjava/lang/String;)V");
        info_js = (jstring) env->NewObject(cls, ctor, array, strEncode);
        path_js = env->NewStringUTF(g_crash_info->path);
    }

    if (g_crash_callback_method != nullptr) {
        if (info_js == nullptr) {
            info_js = error_js;
        }
        if (path_js == nullptr) {
            path_js = error_path_js;
        }
        jstring crashThreadName_js = env->NewStringUTF(g_crash_info->crash_thread_name);
        env->CallVoidMethod(g_crash_callback_obj, g_crash_callback_method,
                            path_js, info_js, crashThreadName_js);
    } else {
        ALOGE("threadHandler g_crash_callback_method == null");
    }
}

/**
 * 当发生异常后，该线程会被执行
 */
void *CallBackThreadHandler(void *argv) {
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
    notify2Java(env);
    if (needAttach) {
        g_jvm->DetachCurrentThread();
    }
    sem_post(&g_done_semaphore);
    delete g_crash_info;
    return nullptr;
}

static void initCallbackThreadHanlder() {
    sem_init(&g_cat_java_crash_semaphore, 0, 0);
    sem_init(&g_done_semaphore, 0, 0);
    //崩溃时开另一个线程回调上层java方法
    pthread_t thd;
    int ret = pthread_create(&thd, NULL, CallBackThreadHandler, NULL);
    if (ret) {
        ALOGE("%s", "pthread_create error");
    }
}

static void initCallbackObject(JNIEnv *env, jobject callback) {
    g_crash_callback_obj = env->NewGlobalRef(callback);
    jclass cls = env->GetObjectClass(g_crash_callback_obj);
    if (cls == NULL) {
        ALOGE("CallBackThreadHandler get class error");
        return;
    }
    g_crash_callback_method = env->GetMethodID(cls, "onCrash",
                                               "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    if (g_crash_callback_method == NULL) {
        ALOGE("CallBackThreadHandler get jmethodId error");
    }
}

static void initNdkLoader(JNIEnv *env) {
    int versionCode = babyte::getSDKVersion();
    if (versionCode >= babyte::ANDROID_N) {
        ndk_init(env);
    }
}

void just_2_java() {
    google_breakpad::MinidumpDescriptor descriptor(
            google_breakpad::MinidumpDescriptor::kMinidumpJustJava);
    static google_breakpad::ExceptionHandler eh(descriptor, nullptr, NULL, NULL,
                                                true, -1);
    eh.set_whole_callback(DumpCallback);
}

void dump_and_2_java(JNIEnv *env, jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);
    google_breakpad::MinidumpDescriptor descriptor(path);
    static google_breakpad::ExceptionHandler eh(descriptor, nullptr, NULL, NULL,
                                                true, -1);
    eh.set_whole_callback(DumpCallback);
    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_babyte_breakpad_BaByteBreakpad_initBreakpadNative(JNIEnv *env, jobject thiz, jstring path_,
                                                           jobject callback) {
    initCallbackObject(env, callback);
    initNdkLoader(env);
    if (path_ != nullptr) {
        dump_and_2_java(env, path_);
    } else {
        just_2_java();
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    initCallbackThreadHanlder();
    JNIEnv *env;
    g_jvm = vm;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_6;
}