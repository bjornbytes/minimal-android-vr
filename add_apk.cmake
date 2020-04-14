function(add_apk _TARGET_NAME)

    cmake_parse_arguments(_add_apk
      ""
      "MANIFEST;KEYSTORE;KEYSTORE_PASS;KEY_PASS"
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
    set(_APK_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_TARGET_NAME}.dir")
    set(_APK_STAGE "${_APK_BUILD_DIR}/stage")

    set(_JAVA_CLASS_FILES)
    set(_JAVA_COMPILE_FILES)
    foreach(_JAVA_SOURCE_FILE IN LISTS _add_apk_SOURCES)
        get_filename_component(_JAVA_FILE ${_JAVA_SOURCE_FILE} NAME_WE)
        get_filename_component(_JAVA_PATH ${_JAVA_SOURCE_FILE} PATH)
        get_filename_component(_JAVA_FULL ${_JAVA_SOURCE_FILE} ABSOLUTE)
        file(RELATIVE_PATH _JAVA_REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${_JAVA_FULL})
        get_filename_component(_JAVA_REL_PATH ${_JAVA_REL_PATH} PATH)

        list(APPEND _JAVA_COMPILE_FILES ${_JAVA_SOURCE_FILE})
        set(_JAVA_CLASS_FILE "${_APK_STAGE}/${_JAVA_REL_PATH}/${_JAVA_FILE}.class")
        set(_JAVA_CLASS_FILES ${_JAVA_CLASS_FILES} ${_JAVA_CLASS_FILE})
    endforeach()

    # Compile the java files and create a list of class files
    add_custom_command(
        OUTPUT ${_APK_BUILD_DIR}/java_compiled_${_TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E make_directory 
            "${_APK_STAGE}"
        COMMAND ${Java_JAVAC_EXECUTABLE}
            ${CMAKE_JAVA_COMPILE_FLAGS}
            -classpath "${CMAKE_JAVA_INCLUDE_PATH_FINAL}"
            -d ${_APK_BUILD_DIR}
            ${_JAVA_COMPILE_FILES}
        COMMAND ${CMAKE_COMMAND} -E touch ${_APK_BUILD_DIR}/java_compiled_${_TARGET_NAME}
        DEPENDS ${_JAVA_COMPILE_FILES} ${CMAKE_JAVA_INCLUDE_PATH}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Java objects for ${_TARGET_NAME}.apk"
        VERBATIM
    )

    # Create dex file
    add_custom_command(
        OUTPUT ${_APK_STAGE}/classes.dex
        COMMAND ${ANDROID_DEX} 
            --dex
            --output=${_APK_STAGE}/classes.dex
            ${_APK_BUILD_DIR}
        DEPENDS ${_APK_BUILD_DIR}/java_compiled_${_TARGET_NAME}
        WORKING_DIRECTORY ${_APK_BUILD_DIR}
        COMMENT "Compiling DEX for ${_TARGET_NAME}.apk"
        VERBATIM
    )

    # Create unaligned apk
    add_custom_command(
        OUTPUT ${_APK_BUILD_DIR}/${_TARGET_NAME}.unaligned.apk
        COMMAND ${CMAKE_COMMAND} -E copy
            ${_add_apk_RESOURCES} ${_APK_STAGE}
        COMMAND ${ANDROID_AAPT} 
            package
            -M ${CMAKE_CURRENT_SOURCE_DIR}/${_add_apk_MANIFEST}
            -I ${_add_apk_INCLUDE_JARS}
            -F ${_APK_BUILD_DIR}/${_TARGET_NAME}.unaligned.apk
        COMMAND ${ANDROID_AAPT} 
            add
            -f ${_APK_BUILD_DIR}/${_TARGET_NAME}.unaligned.apk
            classes.dex
            libapp.so
        DEPENDS ${_APK_STAGE}/classes.dex
        WORKING_DIRECTORY ${_APK_STAGE}
        COMMENT "Packaging unaligned APK for ${_TARGET_NAME}.apk"
        VERBATIM
    )

    # Align apk into an unsigned apk
    add_custom_command(
        OUTPUT ${_APK_BUILD_DIR}/${_TARGET_NAME}.unsigned.apk
        COMMAND ${ANDROID_ZIPALIGN} 
            -f 4
            ${_APK_BUILD_DIR}/${_TARGET_NAME}.unaligned.apk
            ${_APK_BUILD_DIR}/${_TARGET_NAME}.unsigned.apk
        DEPENDS ${_APK_BUILD_DIR}/${_TARGET_NAME}.unaligned.apk
        COMMENT "Aligning APK for ${_TARGET_NAME}.apk"
        VERBATIM
    )

    
    # Sign apk
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_TARGET_NAME}.apk
        COMMAND ${ANDROID_APKSIGNER} sign
            --ks ${_add_apk_KEYSTORE}
            --ks-pass ${_add_apk_KEYSTORE_PASS}
            --key-pass ${_add_apk_KEY_PASS}
            --out ${CMAKE_CURRENT_BINARY_DIR}/${_TARGET_NAME}.apk
            ${_APK_BUILD_DIR}/${_TARGET_NAME}.unsigned.apk
        DEPENDS ${_APK_BUILD_DIR}/${_TARGET_NAME}.unsigned.apk
        COMMENT "Signing APK for ${_TARGET_NAME}.apk"
        VERBATIM
    )

    add_custom_target(${_TARGET_NAME} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_TARGET_NAME}.apk)

endfunction()