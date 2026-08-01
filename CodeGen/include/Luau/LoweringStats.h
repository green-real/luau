// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace Luau
{
namespace CodeGen
{

struct BlockLinearizationStats
{
    unsigned int constPropInstructionCount = 0;
    double timeSeconds = 0.0;

    BlockLinearizationStats& operator+=(const BlockLinearizationStats& that)
    {
        this->constPropInstructionCount += that.constPropInstructionCount;
        this->timeSeconds += that.timeSeconds;

        return *this;
    }

    BlockLinearizationStats operator+(const BlockLinearizationStats& other) const
    {
        BlockLinearizationStats result(*this);
        result += other;
        return result;
    }
};

enum FunctionStatsFlags
{
    // Enable stats collection per function
    FunctionStats_Enable = 1 << 0,
    // Compute function bytecode summary
    FunctionStats_BytecodeSummary = 1 << 1,
    // Collect the per-command, def-use and representation counters below
    FunctionStats_IrDetail = 1 << 2,
};

// Which path a block sits on. Counting a fast-path instruction and a deopt-path one in the same bucket hides the
// thing worth knowing, because the second only runs when the first has already given up.
enum IrBlockClass
{
    // Bytecode, Internal and Linearized: what a run that never leaves native executes
    IrBlockClass_Fast,
    // Fallback: the VM helper path a guard drops into
    IrBlockClass_Fallback,
    // ExitSync: register sync written before handing control back to the interpreter
    IrBlockClass_ExitSync,

    IrBlockClass_Count,
};

// IrCmd is a uint8_t, so this covers every command without depending on which one happens to be declared last.
inline constexpr size_t kIrCmdSlots = 256;
inline constexpr size_t kIrOpKindSlots = 16;
inline constexpr size_t kIrValueKindSlots = 16;
inline constexpr size_t kUseCountBuckets = 6;

// Counters over the IR the backend actually consumes, collected after optimization and before lowering.
//
// Every vector stays empty unless FunctionStats_IrDetail is set, so a normal compile allocates nothing and the
// merge below is a no-op. Answering these questions by matching text against an IR dump does not work: the dump
// omits exit blocks, hides which value an operand refers to, and cannot tell a result-producing instruction from
// a store.
struct IrDetailStats
{
    // [command][block class]
    std::vector<uint32_t> cmdByClass;
    // [defining command][consuming command]: an edge per operand that references another instruction's result.
    // This is the question adjacency in a dump cannot answer, because scheduling is not dataflow.
    std::vector<uint32_t> defUse;
    // [value kind][block class], the representation each result is held in
    std::vector<uint32_t> valueKindByClass;
    // [operand kind], counted over every operand of every live instruction
    std::vector<uint32_t> opKind;
    // Results bucketed by how many times they are used: 0, 1, 2, 3, 4-7, 8+
    std::vector<uint32_t> useCountBuckets;
    // [block class]
    std::vector<uint32_t> blocksByClass;

    void ensure()
    {
        if (cmdByClass.empty())
        {
            cmdByClass.resize(kIrCmdSlots * IrBlockClass_Count);
            defUse.resize(kIrCmdSlots * kIrCmdSlots);
            valueKindByClass.resize(kIrValueKindSlots * IrBlockClass_Count);
            opKind.resize(kIrOpKindSlots);
            useCountBuckets.resize(kUseCountBuckets);
            blocksByClass.resize(IrBlockClass_Count);
        }
    }

    bool empty() const
    {
        return cmdByClass.empty();
    }

    static void addInto(std::vector<uint32_t>& target, const std::vector<uint32_t>& source)
    {
        if (source.empty())
            return;

        if (target.size() < source.size())
            target.resize(source.size());

        for (size_t i = 0; i < source.size(); i++)
            target[i] += source[i];
    }

    IrDetailStats& operator+=(const IrDetailStats& that)
    {
        addInto(cmdByClass, that.cmdByClass);
        addInto(defUse, that.defUse);
        addInto(valueKindByClass, that.valueKindByClass);
        addInto(opKind, that.opKind);
        addInto(useCountBuckets, that.useCountBuckets);
        addInto(blocksByClass, that.blocksByClass);

        return *this;
    }
};

struct FunctionStats
{
    std::string name;
    int line = -1;
    unsigned bcodeCount = 0;
    unsigned irCount = 0;
    unsigned asmCount = 0;
    unsigned asmSize = 0;
    std::vector<std::vector<unsigned>> bytecodeSummary;
};

struct LoweringStats
{
    unsigned totalFunctions = 0;
    unsigned skippedFunctions = 0;
    int spillsToSlot = 0;
    int spillsToRestore = 0;
    unsigned maxSpillSlotsUsed = 0;
    unsigned blocksPreOpt = 0;
    unsigned blocksPostOpt = 0;
    unsigned maxBlockInstructions = 0;

    int regAllocErrors = 0;
    int loweringErrors = 0;

    BlockLinearizationStats blockLinearizationStats;

    IrDetailStats irDetail;

    unsigned functionStatsFlags = 0;
    std::vector<FunctionStats> functions;

    LoweringStats operator+(const LoweringStats& other) const
    {
        LoweringStats result(*this);
        result += other;
        return result;
    }

    LoweringStats& operator+=(const LoweringStats& that)
    {
        this->totalFunctions += that.totalFunctions;
        this->skippedFunctions += that.skippedFunctions;
        this->spillsToSlot += that.spillsToSlot;
        this->spillsToRestore += that.spillsToRestore;
        this->maxSpillSlotsUsed = std::max(this->maxSpillSlotsUsed, that.maxSpillSlotsUsed);
        this->blocksPreOpt += that.blocksPreOpt;
        this->blocksPostOpt += that.blocksPostOpt;
        this->maxBlockInstructions = std::max(this->maxBlockInstructions, that.maxBlockInstructions);

        this->regAllocErrors += that.regAllocErrors;
        this->loweringErrors += that.loweringErrors;

        this->blockLinearizationStats += that.blockLinearizationStats;

        this->irDetail += that.irDetail;

        if (this->functionStatsFlags & FunctionStats_Enable)
            this->functions.insert(this->functions.end(), that.functions.begin(), that.functions.end());

        return *this;
    }
};

} // namespace CodeGen
} // namespace Luau
