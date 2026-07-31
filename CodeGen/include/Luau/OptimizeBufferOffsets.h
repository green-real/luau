// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#pragma once

#include <stdint.h>
#include <vector>

namespace Luau
{
namespace CodeGen
{

struct IrBuilder;

// Which addressing modes decide whether a displacement can be folded
enum class BufferOffsetTarget
{
    X64,
    A64,
};

// Rewrites 'BUFFER_READF32 %buf, (ADD_INT %base, 4i)' into 'BUFFER_READF32 %buf, %base, 4i', killing adds that CSE
// left spanning a whole structure copy. Must run before updateLastUseLocations: folding extends the live range of
// the base up to the last access folded onto it.
void optimizeBufferOffsets(IrBuilder& build, const std::vector<uint32_t>& sortedBlocks, BufferOffsetTarget target);

} // namespace CodeGen
} // namespace Luau
