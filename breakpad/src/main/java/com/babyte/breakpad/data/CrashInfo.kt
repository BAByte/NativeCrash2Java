package com.babyte.breakpad.data

import androidx.annotation.Keep

@Keep
data class CrashInfo(
    val path: String?,
    val nativeInfo: String,
    val nativeThreadTrack: String,
    val jvmThreadTrack: String
)
