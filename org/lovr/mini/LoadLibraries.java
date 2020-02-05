package org.lovr.mini;

import android.app.NativeActivity;

public class LoadLibraries extends NativeActivity {
  static {
    System.loadLibrary("app");
    System.loadLibrary("vrapi");
  }
}
