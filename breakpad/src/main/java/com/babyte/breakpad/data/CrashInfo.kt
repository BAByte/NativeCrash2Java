package com.babyte.breakpad.data

import androidx.annotation.Keep

@Keep
data class CrashInfo(val path: String?, val info: String, val jvmThreadTrack: String)
