package com.babyte.breakpad

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
    val callback = object : NativeCrashCallback {
        override fun onCrash(info: String) {
            println("NativeCrashCallback :$info")
        }
    }

    external fun initBreakpadNative(path: String, callback: NativeCrashCallback)

    fun initBreakpad(path: String) {
        initBreakpadNative(path, callback)
        Thread.currentThread()
    }
}