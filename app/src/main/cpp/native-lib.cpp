#include <jni.h>
#include <string>

/**
 * 引起 crash
 */
void Crash() {
    volatile int *a = (int *) (NULL);
    *a = 1;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_babyte_banativecrash_MainActivity_nativeCrash(JNIEnv *env, jobject thiz) {
    Crash();
}