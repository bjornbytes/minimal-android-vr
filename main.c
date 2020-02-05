#include <VrApi.h>
#include <VrApi_Helpers.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <string.h>

static struct {
  bool initialized;
  uint32_t width;
  uint32_t height;
  uint32_t msaa;
  GLuint framebuffers[3];
  GLuint textures[3];
  GLuint depthBuffer;
} gl;

typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLsizei samples, GLint baseViewIndex, GLsizei numViews);

// matrices holds the two view matrices followed by two projection matrices (column major)
// index is the current swapchain index (used to select texture, framebuffer, etc.)
static void render(float* matrices[4], uint32_t index) {
  if (!gl.initialized) {
    gl.initialized = true;
    gl.msaa = 4;

    PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC glFramebufferTextureMultisampleMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC) eglGetProcAddress( "glFramebufferTextureMultisampleMultiviewOVR" );

    glGenTextures(1, &gl.depthBuffer);
    glBindTexture(GL_TEXTURE_2D_ARRAY, gl.depthBuffer);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH24_STENCIL8, gl.width, gl.height, 2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glGenFramebuffers(3, gl.framebuffers);
    for (uint32_t i = 0; i < 3; i++) {
      glBindFramebuffer(GL_FRAMEBUFFER, gl.framebuffers[i]);
      glFramebufferTextureMultisampleMultiviewOVR(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, gl.depthBuffer, 0, gl.msaa, 0, 2);
      glFramebufferTextureMultisampleMultiviewOVR(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl.textures[i], 0, gl.msaa, 0, 2);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, gl.framebuffers[index]);
  glClearBufferfv(GL_COLOR, 0, (float[]) { .05f, 0.f, .22f, 1.f });
  glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.f, 0);

  //

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static struct {
  EGLDisplay display;
  EGLContext context;
  EGLSurface surface;
  ovrJava java;
  ovrMobile* session;
  ovrTextureSwapChain* swapchain;
  uint32_t frameIndex;
  ANativeWindow* window;
  bool resumed;
} state;

static void onAppCmd(struct android_app* app, int32_t cmd) {
  switch (cmd) {
    case APP_CMD_RESUME: state.resumed = true; break;
    case APP_CMD_PAUSE: state.resumed = false; break;
    case APP_CMD_INIT_WINDOW: state.window = app->window; break;
    case APP_CMD_TERM_WINDOW: state.window = NULL; break;
    default: break;
  }

  if (!state.session && state.window && state.resumed) {
    ovrModeParms parms = vrapi_DefaultModeParms(&state.java);
    parms.Flags &= ~VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;
    parms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
    parms.Flags |= VRAPI_MODE_FLAG_FRONT_BUFFER_SRGB;
    parms.Display = (size_t) state.display;
    parms.WindowSurface = (size_t) state.window;
    parms.ShareContext = (size_t) state.context;
    state.session = vrapi_EnterVrMode(&parms);
    state.frameIndex = 0;
    __android_log_print(ANDROID_LOG_DEBUG, "APP", "in vr mode");
  } else if (state.session && (!state.window || !state.resumed)) {
    vrapi_LeaveVrMode(state.session);
    state.session = NULL;
    __android_log_print(ANDROID_LOG_DEBUG, "APP", "out of vr mode");
  }
}

void android_main(struct android_app* app) {
  app->onAppCmd = onAppCmd;

  // EGL //

  if (!(state.display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) || !eglInitialize(state.display, NULL, NULL)) {
    __android_log_print(ANDROID_LOG_DEBUG, "APP", "Could not initialize display");
    return;
  }

  EGLConfig configs[1024];
  EGLint configCount;
  if (!eglGetConfigs(state.display, configs, sizeof(configs) / sizeof(configs[0]), &configCount)) {
    __android_log_print(ANDROID_LOG_DEBUG, "APP", "Could not get EGL configs");
    return;
  }

  const EGLint attributes[] = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0, EGL_SAMPLES, 0, EGL_NONE };
  const EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
  const EGLint surfaceAttributes[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };

  EGLConfig config = 0;
  for (EGLint i = 0; i < configCount && !config; i++) {
    EGLint value, mask;
    EGLint rmask = EGL_OPENGL_ES3_BIT_KHR;
    EGLint smask = EGL_PBUFFER_BIT | EGL_WINDOW_BIT;

    if (!eglGetConfigAttrib(state.display, configs[i], EGL_RENDERABLE_TYPE, &value) || (value & rmask) != rmask) { continue; }
    if (!eglGetConfigAttrib(state.display, configs[i], EGL_SURFACE_TYPE, &value) || (value & smask) != smask) { continue; }

    for (int a = 0; a < sizeof(attributes) / sizeof(attributes[0]); a += 2) {
      if (attributes[a] == EGL_NONE) { config = configs[i]; break; }
      if (!eglGetConfigAttrib(state.display, configs[i], attributes[a], &value) || value != attributes[a + 1]) { break; }
    }
  }

  state.context = eglCreateContext(state.display, config, EGL_NO_CONTEXT, contextAttributes);
  state.surface = eglCreatePbufferSurface(state.display, config, surfaceAttributes);
  if (!state.context || !state.surface) {
    __android_log_print(ANDROID_LOG_DEBUG, "APP", "Could not initialize context");
    return;
  }

  eglMakeCurrent(state.display, state.surface, state.surface, state.context);

  // VRAPI //

  state.java.Vm = app->activity->vm;
  state.java.ActivityObject = app->activity->clazz;
  (*app->activity->vm)->AttachCurrentThread(app->activity->vm, &state.java.Env, NULL);

  ovrInitParms parms = vrapi_DefaultInitParms(&state.java);
  parms.GraphicsAPI = VRAPI_GRAPHICS_API_OPENGL_ES_3;

  if (vrapi_Initialize(&parms) != VRAPI_INITIALIZE_SUCCESS) {
    __android_log_print(ANDROID_LOG_DEBUG, "APP", "Could not initialize vrapi");
  }

  uint32_t width = vrapi_GetSystemPropertyInt(&state.java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
  uint32_t height = vrapi_GetSystemPropertyInt(&state.java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);
  state.swapchain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D_ARRAY, GL_SRGB8_ALPHA8, width, height, 1, 3);

  gl.width = width;
  gl.height = height;
  for (uint32_t i = 0; i < 3; i++) {
    gl.textures[i] = vrapi_GetTextureSwapChainHandle(state.swapchain, i);
  }

  // LOOP //

  while (!app->destroyRequested) {
    int events;
    struct android_poll_source* source;
    while (ALooper_pollAll(state.session ? 0 : -1, NULL, &events, (void**) &source) >= 0) {
      if (source) {
        source->process(app, source);
      }
    }

    if (!state.session) {
      continue;
    }

    double t = vrapi_GetPredictedDisplayTime(state.session, state.frameIndex);
    ovrTracking2 tracking = vrapi_GetPredictedTracking2(state.session, t);

    ovrMatrix4f ms[] = {
      [0] = ovrMatrix4f_Transpose(&tracking.Eye[0].ViewMatrix),
      [1] = ovrMatrix4f_Transpose(&tracking.Eye[1].ViewMatrix),
      [2] = ovrMatrix4f_Transpose(&tracking.Eye[0].ProjectionMatrix),
      [3] = ovrMatrix4f_Transpose(&tracking.Eye[1].ProjectionMatrix)
    };

    float* matrices[4] = { &ms[0].M[0][0], &ms[1].M[0][0], &ms[2].M[0][0], &ms[3].M[0][0] };

    render(matrices, state.frameIndex % 3);

    ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
    layer.HeadPose = tracking.HeadPose;
    layer.Textures[0].ColorSwapChain = state.swapchain;
    layer.Textures[0].SwapChainIndex = state.frameIndex % 3;
    layer.Textures[0].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&tracking.Eye[0].ProjectionMatrix);
    memcpy(&layer.Textures[1], &layer.Textures[0], sizeof(layer.Textures[0]));
    layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

    vrapi_SubmitFrame2(state.session, &(ovrSubmitFrameDescription2) {
      .FrameIndex = state.frameIndex++,
      .DisplayTime = t,
      .LayerCount = 1,
      .Layers = (const ovrLayerHeader2*[1]) { &layer.Header }
    });
  }

  // BYE //

  vrapi_DestroyTextureSwapChain(state.swapchain);
  vrapi_Shutdown();
  (*app->activity->vm)->DetachCurrentThread(app->activity->vm);
  eglMakeCurrent(state.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface(state.display, state.surface);
  eglDestroyContext(state.display, state.context);
  eglTerminate(state.display);
}
