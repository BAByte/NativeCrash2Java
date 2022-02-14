package com.babyte.banativecrash

import android.os.Bundle
import android.util.Log
import android.widget.Button
import androidx.appcompat.app.AppCompatActivity
import com.babyte.breakpad.BaByteBreakpad
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import java.lang.StringBuilder

class MainActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "crash"
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
            //格式化输出到控制台
            BaByteBreakpad.formatPrint(TAG, info)
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