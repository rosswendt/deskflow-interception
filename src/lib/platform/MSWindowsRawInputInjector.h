/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#if defined(_WIN32)

//! Attempt to inject a relative mouse move using the raw input path.
//! Returns true if the injection succeeds.
bool sendMouseRelativeRawInput(int dx, int dy);

//! Attempt to inject a keyboard event using the raw input path.
//! Returns true if the injection succeeds.
bool sendKeyboardRawInput(WORD vk, WORD scan, DWORD flags);

#endif

