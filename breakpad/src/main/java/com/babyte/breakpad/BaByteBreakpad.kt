package com.babyte.breakpad

import androidx.annotation.Keep
import com.babyte.breakpad.callback.NativeCrashCallback
import com.babyte.breakpad.data.CrashInfo


/**
 * 不能修改包名
 */
@Keep
object BaByteBreakpad {
    init {
        System.loadLibrary("breakpad")
    }

    @JvmStatic

    private external fun initBreakpadNative(path: String?, callback: NativeCrashCallback)

    /**
     * @param dir 输出minidump文件到dir目录
     * @param nativeCrashWholeCallBack 将minidump文件路径，native层的异常信息，java层异常信息回调给开发者
     */
    fun initBreakpad(dir: String, nativeCrashWholeCallBack: (CrashInfo) -> Unit) {
        initBreakpadNative(dir, object : NativeCrashCallback {
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