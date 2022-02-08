package com.babyte.breakpad

import android.util.Log
import androidx.annotation.Keep
import com.babyte.breakpad.callback.NativeCrashCallback


/**
 * 不能修改包名
 */
@Keep
object BaByteBreakpad {
    init {
        System.loadLibrary("breakpad")
    }

    @JvmStatic
    private val nativeCrashCallback = object : NativeCrashCallback {
        override fun onCrash(miniDumpPath: String, info: String) {
            Log.e("NativeCrashCallback",info)
            for ((thread,stack) in Thread.getAllStackTraces()) {
                println("NativeCrashCallback thread = ${thread.name}")
            }
        }
    }

    external fun initBreakpadNative(path: String, callback: NativeCrashCallback)

    fun initBreakpad(path: String) {
        initBreakpadNative(path, nativeCrashCallback)
        Thread.currentThread()
    }
}