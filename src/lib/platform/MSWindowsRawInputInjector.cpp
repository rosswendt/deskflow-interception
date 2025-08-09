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

typedef struct _KEYBOARD_INPUT_DATA
{
  USHORT UnitId;
  USHORT MakeCode;
  USHORT Flags;
  USHORT Reserved;
  ULONG ExtraInformation;
} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

namespace {

using NtUserInjectMouseInputFn = LONG(WINAPI *)(ULONG, PMOUSE_INPUT_DATA, ULONG);
using NtUserInjectKeyboardInputFn = LONG(WINAPI *)(ULONG, PKEYBOARD_INPUT_DATA, ULONG);

struct RawInjector {
  NtUserInjectMouseInputFn injectMouse = nullptr;
  NtUserInjectKeyboardInputFn injectKeyboard = nullptr;
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
    g_injector.injectKeyboard = reinterpret_cast<NtUserInjectKeyboardInputFn>(
        GetProcAddress(lib, "NtUserInjectKeyboardInput"));
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

bool sendKeyboardRawInput(WORD vk, WORD scan, DWORD flags)
{
  (void)vk;
  initInjector();
  if (g_injector.injectKeyboard == nullptr) {
    return false;
  }

  // The raw input path expects scancodes; fall back if unicode events are
  // requested.
  if ((flags & KEYEVENTF_UNICODE) != 0) {
    return false;
  }

  KEYBOARD_INPUT_DATA data{};
  data.UnitId = 0;
  data.MakeCode = scan;
  data.Flags = 0;
  if (flags & KEYEVENTF_KEYUP) {
    data.Flags |= 0x1; // KEY_BREAK
  }
  if (flags & KEYEVENTF_EXTENDEDKEY) {
    data.Flags |= 0x2; // KEY_E0
  }
  data.Reserved = 0;
  data.ExtraInformation = 0;

  LONG status = g_injector.injectKeyboard(1, &data, sizeof(data));
  return status >= 0;
}

#endif // _WIN32


