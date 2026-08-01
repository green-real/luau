// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#pragma once

#include <string>

namespace Luau
{
namespace CodeGen
{

struct IrDetailStats;

// Turn the raw counters into the findings they are collected for: what the fast path is made of, how much of the
// output exists only to leave native code, which conversions are paid for and which of those are round trips, and
// what representation every value is held in.
//
// The analysis lives here rather than in a consumer because the questions are properties of the IR, not of whoever
// happens to be reading it, and because deriving them from printed text is what this whole facility replaces.
std::string reportIrDetail(const IrDetailStats& detail);

} // namespace CodeGen
} // namespace Luau
