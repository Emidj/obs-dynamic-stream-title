// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
namespace dst {
// libobs packs the major in bits 24..31, minor in 16..23, patch in 0..15.
constexpr bool supportedObsVersion(std::uint32_t version) noexcept
{
 return (version >> 24) == 32;
}
}
