#include <jni.h>
#include <string>

void Crash() {
    volatile int *a = (int *) nullptr;
    *a = 1;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_babyte_banativecrash_MainActivity_nativeCrash(JNIEnv *env, jobject thiz) {
    Crash();
}