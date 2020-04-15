Minimal Android VR
===

A tiny test app for running C code on an Oculus Android VR device.  Used for doing quick experiments
or generating minimal test cases for bug reports.

It doesn't use gradle or Android Studio, instead it is built with command line tools and tup or cmake 
is used to automate some things.

Used to prototype how we could build Lovr in the future, but it's unclear yet if this is a good
approach. YMMW.

Usage (Tup)
---

### Requirements

- Oculus Mobile SDK
- Android SDK and NDK (I don't know which versions will work)
- tup

### Setup

- Edit tup.config to set up various paths and versions (this is the hard part)
- Create a keystore (see tup.config)
- Run `tup init` in the root

### Building

Run `tup`

### Deploying

tup outputs an `app.apk` file.  You can use `adb install app.apk` to install it to the device.  I
usually run the following command to build, install, and tail logs in one step:

```sh
tup && adb install -r app.apk && adb logcat -s APP
```

Usage (CMake)
----

### Requirements

- cmake
- Android SDK and NDK (I don't know which versions will work)

### Setup

To generate a keystore, do this:
`keytool -genkey -v -keystore YOURNAME-key.jks -keyalg RSA -keysize 2048 -validity 10000 -alias YOURNAME`


#### Windows

- cmake >= 3.18 (contains fix for USE_JAVA in mingw)
- mingw bash
- make
- Android Studio

```
mkdir build; cd build; \
cmake \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/AppData/Local/Android/Sdk/ndk-bundle/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=29 -DANDROID_BUILD_TOOLS_VERSION=29.0.0 \
    -DJAVA_HOME="C:/Program Files/Android/Android Studio/jre" \
    -DKEYSTORE=~/.key
    -DKEYSTORE_PASS=pass:1234
    -DKEY_PASS=pass:1234
    -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles" ..`
```

#### MacOS

- Xcode (or at least commandline developer tools)

```
mkdir build; cd build; 
cmake \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/Library/Android/Sdk/ndk-bundle/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=29 -DANDROID_BUILD_TOOLS_VERSION=29.0.0 \
    -DKEYSTORE=~/.key
    -DKEYSTORE_PASS=pass:1234
    -DKEY_PASS=pass:1234
    -G "Unix Makefiles" ..
```

### Building

1. `make minimalvr`

### Deploying

Make outputs a `minimal.apk` file.  You can use `adb install minimalvr.apk` to install it to the device.  I
usually run the following command to build, install, and tail logs in one step:

```sh
make minimalvr && adb install -r minimalvr.apk && adb logcat -s APP
```