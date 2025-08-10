// wix-custom.cpp
// ASCII only

#include "wix-custom.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Msi.h>        // MsiSetPropertyW
#include <MsiQuery.h>

#include <stdio.h>
#include <string>

namespace {
const size_t kLogLineMax = 1024;
const std::string kLogPrefix = std::string("@CMAKE_PROJECT_NAME@") + " installer: ";
static std::string s_log; // reusable buffer
}

#define MS_LOG_DEBUG(fmt, ...)                         \
  do {                                                 \
    s_log.resize(kLogLineMax);                         \
    sprintf(s_log.data(), fmt, __VA_ARGS__);           \
    OutputDebugStringA((kLogPrefix + s_log + "\n").c_str()); \
  } while (0)

// Helpers that always read the x64 registry view
static bool RegReadDWORD64(const wchar_t* subkey, const wchar_t* value, DWORD* out) {
  HKEY hKey = 0;
  LONG rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
  if (rc != ERROR_SUCCESS) return false;
  DWORD type = 0, size = sizeof(DWORD), data = 0;
  rc = RegGetValueW(hKey, 0, value, RRF_RT_REG_DWORD, &type, &data, &size);
  RegCloseKey(hKey);
  if (rc != ERROR_SUCCESS) return false;
  *out = data;
  return true;
}

static bool RegReadString64(const wchar_t* subkey, const wchar_t* value, wchar_t* buf, DWORD* inout_chars) {
  HKEY hKey = 0;
  LONG rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
  if (rc != ERROR_SUCCESS) return false;
  DWORD type = 0;
  DWORD bytes = (*inout_chars) * sizeof(wchar_t);
  rc = RegGetValueW(hKey, 0, value, RRF_RT_REG_SZ, &type, buf, &bytes);
  RegCloseKey(hKey);
  if (rc != ERROR_SUCCESS) return false;
  *inout_chars = bytes / sizeof(wchar_t);
  return true;
}

extern "C" __declspec(dllexport) UINT __stdcall CheckVCRedist(MSIHANDLE hInstall) {
  // default: not ok
  MsiSetPropertyW(hInstall, L"VC_REDIST_VERSION_OK", L"0");

  MS_LOG_DEBUG("checking for msvc redist >= %d.%d", REQ_MSVC_MAJOR, REQ_MSVC_MINOR);

  // Ensure the redist key exists and is marked Installed=1
  DWORD installed = 0;
  if (!RegReadDWORD64(VC_REDIST_REG_KEY, VC_REDIST_VAL_INST, &installed) || installed == 0) {
    MS_LOG_DEBUG("msvc redist not installed or key missing", 0);
    return ERROR_SUCCESS; // let MSI show friendly message via condition
  }

  // Prefer numeric Major/Minor if present
  DWORD major = 0, minor = 0;
  bool haveMaj = RegReadDWORD64(VC_REDIST_REG_KEY, L"Major", &major);
  bool haveMin = RegReadDWORD64(VC_REDIST_REG_KEY, L"Minor", &minor);

  bool ok = false;
  if (haveMaj && haveMin) {
    MS_LOG_DEBUG("found msvc version Major=%lu Minor=%lu", major, minor);
    if (major > REQ_MSVC_MAJOR) ok = true;
    else if (major == REQ_MSVC_MAJOR && minor >= REQ_MSVC_MINOR) ok = true;
  } else {
    // Fallback to parsing Version string like "v14.44.3335.0"
    wchar_t ver[64] = {0};
    DWORD n = (DWORD)(sizeof(ver) / sizeof(ver[0]));
    if (RegReadString64(VC_REDIST_REG_KEY, VC_REDIST_VAL_VER, ver, &n)) {
      unsigned int vmaj = 0, vmin = 0;
      swscanf_s(ver, L"v%u.%u", &vmaj, &vmin);
      MS_LOG_DEBUG("parsed msvc version string: %u.%u", vmaj, vmin);
      if (vmaj > REQ_MSVC_MAJOR) ok = true;
      else if (vmaj == REQ_MSVC_MAJOR && vmin >= REQ_MSVC_MINOR) ok = true;
    } else {
      MS_LOG_DEBUG("version string not found", 0);
    }
  }

  if (ok) {
    MS_LOG_DEBUG("msvc redist check passed; setting property", 0);
    MsiSetPropertyW(hInstall, L"VC_REDIST_VERSION_OK", L"1");
  } else {
    MS_LOG_DEBUG("msvc redist check failed", 0);
  }

  return ERROR_SUCCESS; // never hard-fail the install; let MSI logic decide
}
