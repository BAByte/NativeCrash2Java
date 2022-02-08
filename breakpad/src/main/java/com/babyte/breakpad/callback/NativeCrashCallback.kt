package com.babyte.breakpad.callback

interface NativeCrashCallback {
    fun onCrash(miniDumpPath: String, info: String)
}