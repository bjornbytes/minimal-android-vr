cmake_minimum_required(VERSION 3.1.0)
project(MinimalAndroidVR VERSION 1.0.0)

find_package(Java 1.8 REQUIRED)
include(UseJava)

get_filename_component(ANDROID_SDK "${ANDROID_NDK}/.." ABSOLUTE)
file(TO_CMAKE_PATH "${ANDROID_SDK}" ANDROID_SDK)
set(ANDROID_GLUE "${ANDROID_NDK}/sources/android/native_app_glue")
set(ANDROID_BUILD_TOOLS "${ANDROID_SDK}/build-tools/${ANDROID_BUILD_TOOLS_VERSION}/")
set(ANDROID_DEX "${ANDROID_BUILD_TOOLS}/dx.bat")
set(ANDROID_AAPT "${ANDROID_BUILD_TOOLS}/aapt")

link_directories("deps/ovr_sdk_mobile/VrApi/Libs/Android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE}")
include_directories("deps/ovr_sdk_mobile/VrApi/Include" "${ANDROID_GLUE}")
add_library(app SHARED main.c "${ANDROID_GLUE}/android_native_app_glue.c")
target_link_libraries(app android vrapi log GLESv3 egl)

function(add_apk _TARGET_NAME)

    cmake_parse_arguments(_add_apk
      ""
      "MANIFEST"
      "SOURCES;RESOURCES;INCLUDE_JARS"
      ${ARGN}
    )

    set(CMAKE_JAVA_INCLUDE_PATH
        ${CMAKE_JAVA_INCLUDE_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_JAVA_OBJECT_OUTPUT_PATH}
        ${CMAKE_JAVA_LIBRARY_OUTPUT_PATH}
    )

    if (CMAKE_HOST_WIN32 AND NOT CYGWIN AND CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
        set(CMAKE_JAVA_INCLUDE_FLAG_SEP ";")
    else ()
        set(CMAKE_JAVA_INCLUDE_FLAG_SEP ":")
    endif()

    foreach(_JAVA_INCLUDE_JAR IN LISTS _add_apk_INCLUDE_JARS)
        list(APPEND CMAKE_JAVA_INCLUDE_PATH "${_JAVA_INCLUDE_JAR}")
    endforeach()
    foreach (JAVA_INCLUDE_DIR IN LISTS CMAKE_JAVA_INCLUDE_PATH)
        string(APPEND CMAKE_JAVA_INCLUDE_PATH_FINAL "${CMAKE_JAVA_INCLUDE_FLAG_SEP}${JAVA_INCLUDE_DIR}")
    endforeach()

    set(CMAKE_JAVA_CLASS_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_TARGET_NAME}.dir")

    set(_JAVA_CLASS_FILES)
    set(_JAVA_COMPILE_FILES)
    foreach(_JAVA_SOURCE_FILE IN LISTS _add_apk_SOURCES)
        get_filename_component(_JAVA_FILE ${_JAVA_SOURCE_FILE} NAME_WE)
        get_filename_component(_JAVA_PATH ${_JAVA_SOURCE_FILE} PATH)
        get_filename_component(_JAVA_FULL ${_JAVA_SOURCE_FILE} ABSOLUTE)
        file(RELATIVE_PATH _JAVA_REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${_JAVA_FULL})
        get_filename_component(_JAVA_REL_PATH ${_JAVA_REL_PATH} PATH)

        list(APPEND _JAVA_COMPILE_FILES ${_JAVA_SOURCE_FILE})
        set(_JAVA_CLASS_FILE "${CMAKE_JAVA_CLASS_OUTPUT_PATH}/${_JAVA_REL_PATH}/${_JAVA_FILE}.class")
        set(_JAVA_CLASS_FILES ${_JAVA_CLASS_FILES} ${_JAVA_CLASS_FILE})
    endforeach()

    # Compile the java files and create a list of class files
    add_custom_command(
        OUTPUT ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/java_compiled_${_TARGET_NAME}
        COMMAND ${Java_JAVAC_EXECUTABLE}
            ${CMAKE_JAVA_COMPILE_FLAGS}
            -classpath "${CMAKE_JAVA_INCLUDE_PATH_FINAL}"
            -d ${CMAKE_JAVA_CLASS_OUTPUT_PATH}
            ${_JAVA_COMPILE_FILES}
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/java_compiled_${_TARGET_NAME}
        DEPENDS ${_JAVA_COMPILE_FILES} ${CMAKE_JAVA_INCLUDE_PATH}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Java objects for ${_TARGET_NAME}.apk"
        VERBATIM
    )

    # Create dex file
    add_custom_command(
        OUTPUT ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/classes.dex
        COMMAND ${ANDROID_DEX} 
            --dex
            --output=${CMAKE_JAVA_CLASS_OUTPUT_PATH}/classes.dex
            ${CMAKE_JAVA_CLASS_OUTPUT_PATH}
        DEPENDS ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/java_compiled_${_TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_JAVA_CLASS_OUTPUT_PATH}
        COMMENT "Compiling DEX for ${_TARGET_NAME}.apk"
        VERBATIM
    )

    # Create unaligned apk
    add_custom_command(
        OUTPUT ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/${_TARGET_NAME}.unaligned.apk
        COMMAND ${ANDROID_AAPT} 
            package
            -M ${CMAKE_CURRENT_SOURCE_DIR}/${_add_apk_MANIFEST}
            -I ${_add_apk_INCLUDE_JARS}
            -F ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/${_TARGET_NAME}.unaligned.apk
        COMMAND ${ANDROID_AAPT} 
            add
            -f ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/${_TARGET_NAME}.unaligned.apk
            classes.dex
            ${_add_apk_RESOURCES}
        DEPENDS ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/classes.dex
        WORKING_DIRECTORY ${CMAKE_JAVA_CLASS_OUTPUT_PATH}
        COMMENT "Packaging unaligned APK for ${_TARGET_NAME}.apk"
        VERBATIM
    )

    add_custom_target(${_TARGET_NAME} ALL DEPENDS ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/${_TARGET_NAME}.unaligned.apk)

endfunction()

add_apk(minimalvr
    SOURCES
        "org/lovr/mini/LoadLibraries.java"
    MANIFEST "AndroidManifest.xml"
    RESOURCES
        $<TARGET_FILE:app>
    INCLUDE_JARS "${ANDROID_SDK}/platforms/android-${ANDROID_PLATFORM}/android.jar"
)