// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../src/common/scm_rev.h"

namespace Common {

constexpr char g_version[]  = "1.0.0";
constexpr bool g_is_release = true;

constexpr char g_scm_rev[]         = "standalone";
constexpr char g_scm_branch[]      = "standalone";
constexpr char g_scm_desc[]        = "standalone";
constexpr char g_scm_remote_name[] = "standalone";
constexpr char g_scm_remote_url[]  = "standalone";
constexpr char g_scm_date[]        = "standalone";

const std::string GetRemoteNameFromLink() {
    return "standalone";
}

} // namespace Common

