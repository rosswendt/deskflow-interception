/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsRawInputInjector.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef struct _MOUSE_INPUT_DATA
{
  USHORT UnitId;
  USHORT Flags;
  union
  {
    ULONG Buttons;
    struct
    {
      USHORT ButtonFlags;
      USHORT ButtonData;
    };
  };
  ULONG RawButtons;
  LONG LastX;
  LONG LastY;
  ULONG ExtraInformation;
} MOUSE_INPUT_DATA, *PMOUSE_INPUT_DATA;

namespace {

using NtUserInjectMouseInputFn = LONG(WINAPI *)(ULONG, PMOUSE_INPUT_DATA, ULONG);

struct RawInjector {
  NtUserInjectMouseInputFn injectMouse = nullptr;
  bool initialized = false;
};

RawInjector g_injector;

void initInjector()
{
  if (g_injector.initialized) {
    return;
  }
  g_injector.initialized = true;

  if (HMODULE lib = LoadLibraryW(L"win32u.dll")) {
    g_injector.injectMouse = reinterpret_cast<NtUserInjectMouseInputFn>(
        GetProcAddress(lib, "NtUserInjectMouseInput"));
  }
}

} // namespace

bool sendMouseRelativeRawInput(int dx, int dy)
{
  initInjector();
  if (g_injector.injectMouse == nullptr) {
    return false;
  }

  MOUSE_INPUT_DATA data{};
  data.LastX = dx;
  data.LastY = dy;
  data.ButtonFlags = 0;
  data.UnitId = 0;
  data.Flags = 0; // relative move
  LONG status = g_injector.injectMouse(1, &data, sizeof(data));
  return status >= 0;
}

#endif // _WIN32


