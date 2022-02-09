package com.babyte.breakpad

import androidx.annotation.Keep
import com.babyte.breakpad.callback.NativeCrashCallback
import com.babyte.breakpad.data.CrashInfo
import java.lang.StringBuilder


/**
 * 不能修改包名
 */
@Keep
object BaByteBreakpad {
    private var isWhole = false

    init {
        System.loadLibrary("breakpad")
    }

    @JvmStatic

    private external fun initBreakpadNative(path: String?, callback: NativeCrashCallback)

    fun initBreakpad(path: String, nativeCrashWholeCallBack: (CrashInfo) -> Unit) {
        initBreakpadNative(path, object : NativeCrashCallback {
            override fun onCrash(miniDumpPath: String, info: String, crashThreadName: String) {
                nativeCrashWholeCallBack.invoke(
                    CrashInfo(
                        miniDumpPath,
                        info,
                        getStack(crashThreadName)
                    )
                )
            }
        })
    }

    fun initBreakpad(nativeCrashInfoCallBack: (CrashInfo) -> Unit) {
        initBreakpadNative(null, object : NativeCrashCallback {
            override fun onCrash(miniDumpPath: String, info: String, crashThreadName: String) {
                nativeCrashInfoCallBack.invoke(
                    CrashInfo(
                        null,
                        info,
                        getStack(crashThreadName)
                    )
                )
            }
        })
    }

    private fun getStack(crashThreadName: String): String {
        val stringBuilder = StringBuilder()
        for ((thread, stack) in Thread.getAllStackTraces()) {
            if (thread.name.contains(crashThreadName)) {
                stringBuilder.appendLine("$thread")
                stack.forEach {
                    stringBuilder.appendLine("     at $it")
                }
            }
        }
        return stringBuilder.toString()
    }

}