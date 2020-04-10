cmake_minimum_required(VERSION 3.1.0)
project(MinimalAndroidVR VERSION 1.0.0)

find_package(Java 1.8 REQUIRED)
include(UseJava)

get_filename_component(ANDROID_SDK "${ANDROID_NDK}/.." ABSOLUTE)
file(TO_CMAKE_PATH "${ANDROID_SDK}" ANDROID_SDK)
set(ANDROID_GLUE "${ANDROID_NDK}/sources/android/native_app_glue")

link_directories("deps/ovr_sdk_mobile/VrApi/Libs/Android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE}")
include_directories("deps/ovr_sdk_mobile/VrApi/Include" "${ANDROID_GLUE}")
add_library(app SHARED main.c "${ANDROID_GLUE}/android_native_app_glue.c")
target_link_libraries(app android vrapi log GLESv3 egl)

add_jar(minimalvr
    SOURCES
        "org/lovr/mini/LoadLibraries.java"
        "AndroidManifest.xml"
        $<TARGET_FILE:app>
    INCLUDE_JARS "${ANDROID_SDK}/platforms/android-${ANDROID_PLATFORM}/android.jar"
)