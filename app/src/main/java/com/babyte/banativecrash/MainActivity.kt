package com.babyte.banativecrash

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.Button
import com.babyte.breakpad.BaByteBreakpad
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "MainActivity"
    }

    init {
        System.loadLibrary("native-lib")
    }

    external fun nativeCrash()
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        //初始化
        BaByteBreakpad.initBreakpad(this.cacheDir.absolutePath) { info ->
            Log.e("BaByteBreakpad", info.path ?: "")
            Log.e("BaByteBreakpad", info.info)
            Log.e("BaByteBreakpad", " \n${info.jvmThreadTrack}")
        }

        val jniButton = findViewById<Button>(R.id.jniCrash)
        jniButton.setOnClickListener {
            GlobalScope.launch(Dispatchers.IO) {
                Log.d(
                    TAG,
                    "jniButton onClick thread = ${Thread.currentThread()} id = ${Thread.currentThread().id}}"
                )
                nativeCrash()
            }
        }
    }
}