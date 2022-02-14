package com.babyte.breakpad

import android.util.Log
import androidx.annotation.Keep
import com.babyte.breakpad.callback.NativeCrashCallback
import com.babyte.breakpad.data.CrashInfo


/**
 * 不能修改包名
 */
@Keep
object BaByteBreakpad {
    private const val LOG_MAX_LENGTH = 20

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
            override fun onCrash(
                miniDumpPath: String,
                crashInfo: String,
                nativeThreadTrack: String,
                crashThreadName: String
            ) {
                nativeCrashWholeCallBack.invoke(
                    CrashInfo(
                        miniDumpPath,
                        crashInfo,
                        nativeThreadTrack,
                        getStack(crashThreadName)
                    )
                )
            }
        })
    }

    fun initBreakpad(nativeCrashInfoCallBack: (CrashInfo) -> Unit) {
        initBreakpadNative(null, object : NativeCrashCallback {
            override fun onCrash(
                miniDumpPath: String,
                crashInfo: String,
                nativeThreadTrack: String,
                crashThreadName: String
            ) {
                nativeCrashInfoCallBack.invoke(
                    CrashInfo(
                        null,
                        crashInfo,
                        nativeThreadTrack,
                        getStack(crashThreadName)
                    )
                )
            }
        })
    }

    fun formatPrint(
        tag: String,
        info:CrashInfo
    ) {
        formatPrint(tag, info.path ?: "dump minidump file failed")
        formatPrint(tag, info.nativeInfo)
        formatPrint(tag, info.nativeThreadTrack)
        formatPrint(tag, info.jvmThreadTrack)
    }

    private fun formatPrint(TAG: String, msg: String) {
        val sb = java.lang.StringBuilder()
        var i = 1
        msg.reader().readLines().forEach {
            i++
            sb.appendLine(it)
            if (i >= LOG_MAX_LENGTH) {
                i = 1
                Log.e(TAG, " \n$sb")
                sb.clear()
            }
        }
        if (sb.isNotEmpty()) {
            Log.e(TAG, " \n$sb")
            sb.clear()
        }
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