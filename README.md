# BANativeCrash
基于breakpad扩展的Android native 异常捕获库
# 功能
+ 保留breakpad导出minidump文件功能
+ 发生异常时将native异常信息，java层的调用栈通过回调提供给开发者，效果如下：
~~~text
------------------- NATIVE: CRASH INFO IN LIBRARY:
    Operating system: Android 28 Linux 4.4.146 #37 SMP PREEMPT Wed Jan 20 18:26:59 CST 2021
    CPU: aarch64 (8 core)
    
    Crash reason: signal 11(SIGSEGV) Invalid address
    Crash address: 0000000000000000
    Crash pc: 0000000000000650
    Crash so: /data/app/com.babyte.banativecrash-B--gNWjJsd2QNL2mGptoEg==/lib/arm64/libnative-lib.so(arm64)
    Crash method: _Z5Crashv
    
    ------------------- CRASH THREAD TRACK:
    Thread[name:DefaultDispatch] (NOTE: linux thread name length limit is 15 characters)
    #00 pc 0000000000000650  /data/app/com.babyte.banativecrash-B--gNWjJsd2QNL2mGptoEg==/lib/arm64/libnative-lib.so (Crash()+20)
    #01 pc 0000000000000670  /data/app/com.babyte.banativecrash-B--gNWjJsd2QNL2mGptoEg==/lib/arm64/libnative-lib.so (Java_com_babyte_banativecrash_MainActivity_nativeCrash+20)
    #02 pc 0000000000565de0  /system/lib64/libart.so (offset 0xc1000) (art_quick_generic_jni_trampoline+144)
    #03 pc 000000000055cd88  /system/lib64/libart.so (offset 0xc1000) (art_quick_invoke_stub+584)
    #04 pc 00000000000cf740  /system/lib64/libart.so (art::ArtMethod::Invoke(art::Thread*, unsigned int*, unsigned int, art::JValue*, char const*)+200)
    #05 pc 00000000002823b8  /system/lib64/libart.so (offset 0xc1000) (art::interpreter::ArtInterpreterToCompiledCodeBridge(art::Thread*, art::ArtMethod*, art::ShadowFrame*, unsigned short, art::JValue*)+344)
    #06 pc 000000000027c374  /system/lib64/libart.so (offset 0xc1000) (bool art::interpreter::DoCall<false, false>(art::ArtMethod*, art::Thread*, art::ShadowFrame&, art::Instruction const*, unsigned short, art::JValue*)+948)
    #07 pc 000000000052c698  /system/lib64/libart.so (offset 0xc1000) (MterpInvokeVirtual+584)
    #08 pc 000000000054f394  /system/lib64/libart.so (offset 0xc1000) (ExecuteMterpImpl+14228)
    #09 pc 000000000003f960  /dev/ashmem/dalvik-classes2.dex extracted in memory from /data/app/com.babyte.banativecrash-B--gNWjJsd2QNL2mGptoEg==/base.apk!classes2.dex (deleted) (com.babyte.banativecrash.MainActivity$onCreate$2$1.invokeSuspend+148)
    #10 pc 0000000000255e70  /system/lib64/libart.so (offset 0xc1000) (_ZN3art11interpreterL7ExecuteEPNS_6ThreadERKNS_20CodeItemDataAccessorERNS_11ShadowFrameENS_6JValueEb.llvm.1181525464+496)
    #11 pc 000000000025b9f0  /system/lib64/libart.so (offset 0xc1000) (art::interpreter::ArtInterpreterToInterpreterBridge(art::Thread*, art::CodeItemDataAccessor const&, art::ShadowFrame*, art::JValue*)+216)
    #12 pc 000000000027c358  /system/lib64/libart.so (offset 0xc1000) (bool art::interpreter::DoCall<false, false>(art::ArtMethod*, art::Thread*, art::ShadowFrame&, art::Instruction const*, unsigned short, art::JValue*)+920)
    #13 pc 000000000052c698  /system/lib64/libart.so (offset 0xc1000) (MterpInvokeVirtual+584)
    #14 pc 000000000054f394  /system/lib64/libart.so (offset 0xc1000) (ExecuteMterpImpl+14228)
    #15 pc 000000000037d7c4  /dev/ashmem/dalvik-classes.dex extracted in memory from /data/app/com.babyte.banativecrash-B--gNWjJsd2QNL2mGptoEg==/base.apk (deleted) (kotlin.coroutines.jvm.internal.BaseContinuationImpl.resumeWith+40)
    #16 pc 0000000000255e70  /system/lib64/libart.so (offset 0xc1000) (_ZN3art11interpreterL7ExecuteEPNS_6ThreadERKNS_20CodeItemDataAccessorERNS_11ShadowFrameENS_6JValueEb.llvm.1181525464+496)
    #17 pc 000000000025b9f0  /system/lib64/libart.so (offset 0xc1000) (art::interpreter::ArtInterpreterToInterpreterBridge(art::Thread*, art::CodeItemDataAccessor const&, art::ShadowFrame*, art::JValue*)+216)
    #18 pc 000000000027c358  /system/lib64/libart.so (offset 0xc1000) (bool art::interpreter::DoCall<false, false>(art::ArtMethod*, art::Thread*, art::ShadowFrame&, art::Instruction const*, unsigned short, art::JValue*)+920)
    #19 pc 000000000052d610  /system/lib64/libart.so (offset 0xc1000) (MterpInvokeInterface+1392)
    #20 pc 000000000054f594  /system/lib64/libart.so (offset 0xc1000) (ExecuteMterpImpl+14740)
    #21 pc 00000000003ba1c2  /dev/ashmem/dalvik-classes.dex extracted in memory from /data/app/com.babyte.banativecrash-B--gNWjJsd2QNL2mGptoEg==/base.apk (deleted) (kotlinx.coroutines.DispatchedTask.run+
    Thread[DefaultDispatcher-worker-2,5,main]
        at java.lang.Object.wait(Native Method)
        at java.lang.Thread.parkFor$(Thread.java:2137)
        at sun.misc.Unsafe.park(Unsafe.java:358)
        at java.util.concurrent.locks.LockSupport.parkNanos(LockSupport.java:353)
        at kotlinx.coroutines.scheduling.CoroutineScheduler$Worker.park(CoroutineScheduler.kt:795)
        at kotlinx.coroutines.scheduling.CoroutineScheduler$Worker.tryPark(CoroutineScheduler.kt:740)
        at kotlinx.coroutines.scheduling.CoroutineScheduler$Worker.runWorker(CoroutineScheduler.kt:711)
        at kotlinx.coroutines.scheduling.CoroutineScheduler$Worker.run(CoroutineScheduler.kt:665)
    Thread[DefaultDispatcher-worker-1,5,main]
        at com.babyte.banativecrash.MainActivity.nativeCrash(Native Method)
        at com.babyte.banativecrash.MainActivity$onCreate$2$1.invokeSuspend(MainActivity.kt:39)
        at kotlin.coroutines.jvm.internal.BaseContinuationImpl.resumeWith(ContinuationImpl.kt:33)
        at kotlinx.coroutines.DispatchedTask.run(DispatchedTask.kt:106)
        at kotlinx.coroutines.scheduling.CoroutineScheduler.runSafely(CoroutineScheduler.kt:571)
        at kotlinx.coroutines.scheduling.CoroutineScheduler$Worker.executeTask(CoroutineScheduler.kt:750)
        at kotlinx.coroutines.scheduling.CoroutineScheduler$Worker.runWorker(CoroutineScheduler.kt:678)
        at kotlinx.coroutines.scheduling.CoroutineScheduler$Worker.run(CoroutineScheduler.kt:665)
    Thread[DefaultDispatcher-worker-3,5,main]
        at java.lang.Object.wait(Native Method)
        at java.lang.Thread.parkFor$(Thread.java:2137)
        at sun.misc.Unsafe.park(Unsafe.java:358)
        at java.util.concurrent.locks.LockSupport.parkNanos(LockSupport.java:353)
        at kotlinx.coroutines.scheduling.CoroutineScheduler$Worker.park(CoroutineScheduler.kt:795)
        at kotlinx.coroutines.scheduling.CoroutineScheduler$Worker.tryPark(CoroutineScheduler.kt:740)
        at kotlinx.coroutines.scheduling.CoroutineScheduler$Worker.runWorker(CoroutineScheduler.kt:711)
        at kotlinx.coroutines.scheduling.CoroutineScheduler$Worker.run(CoroutineScheduler.kt:665)
~~~