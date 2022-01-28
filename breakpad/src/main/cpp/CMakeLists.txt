# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)
set(BREAKPAD_ROOT .)
include_directories(${BREAKPAD_ROOT} ${BREAKPAD_ROOT}/common/android/include)
file(GLOB BREAKPAD_SOURCES_COMMON
        breakpad.cpp
        ${BREAKPAD_ROOT}/third_party/thread/java_thread.cc
        ${BREAKPAD_ROOT}/client/linux/minidump_whole_writer/minidump_whole_track_writer.cc
        ${BREAKPAD_ROOT}/third_party/utils/*.cpp

        ${BREAKPAD_ROOT}/client/linux/crash_generation/crash_generation_client.cc
        ${BREAKPAD_ROOT}/client/linux/dump_writer_common/thread_info.cc
        ${BREAKPAD_ROOT}/client/linux/dump_writer_common/ucontext_reader.cc
        ${BREAKPAD_ROOT}/client/linux/handler/exception_handler.cc
        ${BREAKPAD_ROOT}/client/linux/handler/minidump_descriptor.cc
        ${BREAKPAD_ROOT}/client/linux/log/log.cc
        ${BREAKPAD_ROOT}/client/linux/microdump_writer/microdump_writer.cc
        ${BREAKPAD_ROOT}/client/linux/minidump_writer/linux_dumper.cc
        ${BREAKPAD_ROOT}/client/linux/minidump_writer/linux_ptrace_dumper.cc
        ${BREAKPAD_ROOT}/client/linux/minidump_writer/minidump_writer.cc
        ${BREAKPAD_ROOT}/client/minidump_file_writer.cc
        ${BREAKPAD_ROOT}/common/convert_UTF.cc
        ${BREAKPAD_ROOT}/common/md5.cc
        ${BREAKPAD_ROOT}/common/string_conversion.cc
        ${BREAKPAD_ROOT}/common/linux/elfutils.cc
        ${BREAKPAD_ROOT}/common/linux/file_id.cc
        ${BREAKPAD_ROOT}/common/linux/guid_creator.cc
        ${BREAKPAD_ROOT}/common/linux/linux_libc_support.cc
        ${BREAKPAD_ROOT}/common/linux/memory_mapped_file.cc
        ${BREAKPAD_ROOT}/common/linux/safe_readlink.cc
        )

file(GLOB BREAKPAD_ASM_SOURCE ${BREAKPAD_ROOT}/common/linux/breakpad_getcontext.S)
set_source_files_properties(${BREAKPAD_ASM_SOURCE} PROPERTIES LANGUAGE C)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        breakpad

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${BREAKPAD_SOURCES_COMMON} ${BREAKPAD_ASM_SOURCE} )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        breakpad

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} )
