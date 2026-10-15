/**
 * @file        ui/android_bridge.cpp
 * @brief       JNI glue for the Android build, reached through SDL3.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <rex/ui/android_bridge.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include <android/log.h>
#include <jni.h>
#include <unistd.h>

#include <SDL3/SDL_system.h>

#include <rex/filesystem.h>
#include <rex/logging.h>

namespace rex::ui::android {

namespace {

// Clears a pending Java exception so the next JNI call is legal. Returns true
// when there was one.
bool ClearException(JNIEnv* env) {
  if (!env->ExceptionCheck()) {
    return false;
  }
  env->ExceptionDescribe();
  env->ExceptionClear();
  return true;
}

// activity.getContentResolver().openFileDescriptor(Uri.parse(uri), mode)
//     .detachFd()
int OpenContentUri(std::string_view uri, const char* mode) {
  auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
  auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
  if (!env || !activity) {
    return -1;
  }
  int fd = -1;
  if (env->PushLocalFrame(16) != 0) {
    env->DeleteLocalRef(activity);
    return -1;
  }
  do {
    jclass activity_class = env->GetObjectClass(activity);
    jmethodID get_resolver = env->GetMethodID(activity_class, "getContentResolver",
                                              "()Landroid/content/ContentResolver;");
    jobject resolver = env->CallObjectMethod(activity, get_resolver);
    if (ClearException(env) || !resolver) {
      break;
    }

    jclass uri_class = env->FindClass("android/net/Uri");
    jmethodID uri_parse =
        env->GetStaticMethodID(uri_class, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
    jstring uri_string = env->NewStringUTF(std::string(uri).c_str());
    jobject uri_object = env->CallStaticObjectMethod(uri_class, uri_parse, uri_string);
    if (ClearException(env) || !uri_object) {
      break;
    }

    jclass resolver_class = env->GetObjectClass(resolver);
    jmethodID open_fd = env->GetMethodID(
        resolver_class, "openFileDescriptor",
        "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;");
    jstring mode_string = env->NewStringUTF(mode);
    jobject pfd = env->CallObjectMethod(resolver, open_fd, uri_object, mode_string);
    if (ClearException(env) || !pfd) {
      break;
    }

    jclass pfd_class = env->GetObjectClass(pfd);
    jmethodID detach_fd = env->GetMethodID(pfd_class, "detachFd", "()I");
    fd = env->CallIntMethod(pfd, detach_fd);
    if (ClearException(env)) {
      fd = -1;
    }
  } while (false);
  env->PopLocalFrame(nullptr);
  env->DeleteLocalRef(activity);
  if (fd < 0) {
    REXLOG_ERROR("ContentResolver could not open {} ({})", uri, mode);
  }
  return fd;
}

// An app process has stderr pointed at /dev/null, so whatever libraries
// report there (plume prints every failed Vulkan call that way) would be lost.
// Route it into logcat and the log file instead, one line at a time.
void CaptureStderr() {
  int fds[2];
  if (pipe(fds) != 0) {
    return;
  }
  setvbuf(stderr, nullptr, _IOLBF, 0);
  if (dup2(fds[1], STDERR_FILENO) < 0) {
    close(fds[0]);
    close(fds[1]);
    return;
  }
  close(fds[1]);
  std::thread([read_fd = fds[0]] {
    FILE* in = fdopen(read_fd, "r");
    if (!in) {
      return;
    }
    char* line = nullptr;
    size_t capacity = 0;
    ssize_t length;
    while ((length = getline(&line, &capacity, in)) > 0) {
      if (line[length - 1] == '\n') {
        line[length - 1] = '\0';
      }
      __android_log_write(ANDROID_LOG_WARN, "stderr", line);
      REXLOG_WARN("[stderr] {}", line);
    }
    free(line);
    fclose(in);
  }).detach();
}

}  // namespace

std::filesystem::path GetAppStoragePath() {
  if (SDL_GetAndroidExternalStorageState() & SDL_ANDROID_EXTERNAL_STORAGE_WRITE) {
    if (const char* external = SDL_GetAndroidExternalStoragePath()) {
      return std::filesystem::path(external);
    }
  }
  if (const char* internal = SDL_GetAndroidInternalStoragePath()) {
    return std::filesystem::path(internal);
  }
  return {};
}

void InitializeBridge() {
  CaptureStderr();
  rex::filesystem::SetAndroidContentOpener(&OpenContentUri);

  // Nothing sets HOME for an app process, and GetUserFolder() falls back to the
  // passwd entry, which on Android is not a writable directory.
  std::filesystem::path storage = GetAppStoragePath();
  if (!storage.empty()) {
    setenv("HOME", storage.c_str(), 0);
    setenv("XDG_DATA_HOME", storage.c_str(), 0);
  }
  // There is no /tmp on Android; lock files and temporaries go to the cache.
  if (const char* cache = SDL_GetAndroidCachePath()) {
    setenv("TMPDIR", cache, 0);
    setenv("XDG_RUNTIME_DIR", cache, 0);
  }
}

}  // namespace rex::ui::android
