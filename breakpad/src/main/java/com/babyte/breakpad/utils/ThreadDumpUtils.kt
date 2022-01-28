package com.babyte.breakpad.utils

import androidx.annotation.Keep

@Keep
object ThreadDumpUtils {
    /**
     * 打印当前线程的调用堆栈
     *
     */
    @Keep
    @JvmStatic
    fun getTrackInfo(thread: Thread): String {
        val sb = StringBuffer()
        for (i in thread.stackTrace) {
            if (sb.isNotEmpty()) {
                sb.append("\n")
            }
            sb.append(i)
        }
        return sb.toString()
    }
}