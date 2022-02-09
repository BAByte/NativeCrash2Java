package com.babyte.breakpad.callback

import androidx.annotation.Keep

@Keep
internal interface NativeCrashCallback {
    fun onCrash(miniDumpPath: String, info: String,crashThreadName:String)
}