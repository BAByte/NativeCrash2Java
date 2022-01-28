

#include <jni.h>
#include "java_thread.h"
#include <android/log.h>
#include <third_party/utils/crash_info_.h>
#include <client/linux/minidump_whole_writer/minidump_whole_track_writer.h>

#define LOG_TAG ">>> JavaThread"
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

babyte::JavaThread::JavaThread(JNIEnv *env) {
    thread_cls = env->FindClass("java/lang/Thread");
    if (thread_cls == nullptr) {
        ALOGE("JavaThread thread_cls = null");
    }
    jmethodID mid_currentThread =
            env->GetStaticMethodID(thread_cls, "currentThread", "()Ljava/lang/Thread;");
    if (mid_currentThread == nullptr) {
        ALOGE("JavaThread mid_currentThread = null");
    }
    jobject current_thread = env->CallStaticObjectMethod(thread_cls, mid_currentThread);
    g_current_thread = env->NewGlobalRef(current_thread);
}

/**
 *
 * @param java_thread  need GlobalRef
 */
babyte::JavaThread::JavaThread(jobject java_thread) {
    g_current_thread = java_thread;
}

babyte::JavaThread::JavaThread() {
    //nothing to do
}

babyte::JavaThread::~JavaThread() {
    //nothing to do
}

void babyte::JavaThread::release(JNIEnv *env) {
    if (g_current_thread != nullptr) {
        env->DeleteGlobalRef(g_current_thread);
        g_current_thread = nullptr;
    }
    ALOGD("release g_current_thread");
}

/**
 *
 * @param env
 * @return 失败时返回nullptr
 */
jstring babyte::JavaThread::getName(JNIEnv *env) const {
    //获取线程静态方法getName
    jmethodID mid_getName =
            env->GetMethodID(thread_cls, "getName", "()Ljava/lang/String;");
    if (mid_getName == nullptr) {
        ALOGE("getName mid_getName = null");
        return nullptr;
    }
    return static_cast<jstring>(env->CallObjectMethod(g_current_thread, mid_getName));
}

/**
 * @param env
 * @return 失败时返回-1
 */
jlong babyte::JavaThread::getTid(JNIEnv *env) const {
    //获取线程静态方法getId
    jmethodID mid_getid =
            env->GetMethodID(thread_cls, "getId", "()J");
    if (mid_getid == nullptr) {
        ALOGE("JavaThread getTid mid_getid = null");
        return -1;
    }
    return env->CallLongMethod(g_current_thread, mid_getid);
}

/**
 *
 * @param env
 * @return 失败时返回nullptr
 */
char *babyte::JavaThread::toString(JNIEnv *env) const {
    jmethodID mid_toString = env->GetMethodID(thread_cls, "toString",
                                              "()Ljava/lang/String;");
    if (mid_toString == nullptr) {
        ALOGE("toString mid_toString = null");
        return nullptr;
    }
    jstring info = (jstring) env->CallObjectMethod(g_current_thread, mid_toString);
    return const_cast<char *>(env->GetStringUTFChars(info, 0));
}

/**
 *
 * @param env
 * @return 失败时返回nullptr
 */
char *babyte::JavaThread::getTrackInfo(JNIEnv *env) const {
    jmethodID mid_getStackTrace =
            env->GetMethodID(thread_cls, "getStackTrace", "()[Ljava/lang/StackTraceElement;");
    if (mid_getStackTrace == nullptr) {
        ALOGE("getTrackInfo mid_getid = null");
        return nullptr;
    }
    auto stack_trace_element = (jobjectArray) (env->CallObjectMethod(
            g_current_thread, mid_getStackTrace));
    return splicingTrackInfo(env, stack_trace_element);
}

/**
 *
 * @param env
 * @return 失败时返回nullptr
 */
char *babyte::JavaThread::splicingTrackInfo(JNIEnv *env, jobjectArray stack_trace_element) {
    //获取堆栈信息元素的方法
    jclass stackTraceElement_cls = env->FindClass("java/lang/StackTraceElement");
    if (stackTraceElement_cls == nullptr) {
        ALOGE("splicingTrackInfo stackTraceElement_cls = null");
        return nullptr;
    }
    jmethodID mid_toString = env->GetMethodID(stackTraceElement_cls, "toString",
                                              "()Ljava/lang/String;");
    if (mid_toString == nullptr) {
        ALOGE("splicingTrackInfo mid_toString = null");
        return nullptr;
    }

    auto new_line = "\n";
    char trackInfo[babyte::MAX_LOG_NUM_];
    //拼接
    int size = env->GetArrayLength(stack_trace_element);
    ALOGD("splicingTrackInfo size =  %d", size);
    strcat(trackInfo, new_line);
    for (int i = 3; i < size; ++i) {
        jobject element = env->GetObjectArrayElement(stack_trace_element, i);
        auto info = (jstring) env->CallObjectMethod(element, mid_toString);
        char *info_char = const_cast<char *>(env->GetStringUTFChars(info, 0));
        strcat(trackInfo, info_char);
        env->ReleaseStringUTFChars(info, info_char);
        strcat(trackInfo, new_line);
    }
    return trackInfo;
}


