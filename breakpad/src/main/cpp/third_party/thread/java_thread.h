//
// Created by ba on 1/20/22.
//

#ifndef JAVA_THREAD_H
#define JAVA_THREAD_H

#include <jni.h>

namespace babyte {
    class JavaThread {
    public:
        jobject g_current_thread;
        jclass thread_cls;
        JavaThread();

        JavaThread(JNIEnv *env);

        JavaThread(jobject javathread);

        ~JavaThread();

        void release(JNIEnv *env);

        jstring getName(JNIEnv *env) const;

        jlong getTid(JNIEnv *env) const;

        char * toString(JNIEnv *env) const;

        char * getTrackInfo(JNIEnv *env) const;
    private:
        static char * splicingTrackInfo(JNIEnv *env, jobjectArray stack_trace_element);
    };
}
#endif