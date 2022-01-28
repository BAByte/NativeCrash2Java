package com.babyte.breakpad.callback

interface NativeCrashCallback {
    fun onCrash(info: String)
}