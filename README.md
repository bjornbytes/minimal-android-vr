Minimal Android VR
===

A tiny test app for running C code on an Oculus Android VR device.  Used for doing quick experiments
or generating minimal test cases for bug reports.

It doesn't use gradle or Android Studio, instead it is built with command line tools and tup is used
to automate some things.

Not guaranteed to be fit for any purpose at all, this is just for personal use.

Usage
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
