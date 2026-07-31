// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "Luau/OptimizeBufferOffsets.h"

#include "Luau/IrBuilder.h"
#include "Luau/IrData.h"
#include "Luau/IrUtils.h"

#include "lstate.h"

#include <stdio.h>
#include <stdlib.h>

LUAU_FASTFLAGVARIABLE(LuauCodegenBufferOffsetFold)

namespace Luau
{
namespace CodeGen
{

static int getTagDataOffset(uint8_t tag)
{
    return tag == LUA_TBUFFER ? offsetof(Buffer, data) : tag == LUA_TVECTOR ? offsetof(LuauVector, v) : offsetof(Udata, data);
}

// Mirrors the two immediate forms of AssemblyBuilderA64::placeA. A displacement it cannot encode has no fallback:
// it sets the overflow flag and the whole function drops to bytecode.
static bool isEncodableDisplacement(BufferOffsetTarget target, int effectiveOffset, int accessSize)
{
    if (target == BufferOffsetTarget::X64)
        return effectiveOffset >= -(1 << 24) && effectiveOffset <= (1 << 24);

    int sizelog = accessSize == 8 ? 3 : accessSize == 4 ? 2 : accessSize == 2 ? 1 : 0;

    // Scaled reaches 1023 accesses, but only when non-negative and aligned to the access size
    if (effectiveOffset >= 0 && (effectiveOffset & ((1 << sizelog) - 1)) == 0 && unsigned(effectiveOffset >> sizelog) < 1024)
        return true;

    return effectiveOffset >= -256 && effectiveOffset <= 255;
}

// A check that falls through has established 'base + minOffset >= 0' and 'base + maxOffset <= len' in exact
// arithmetic: its lowering deliberately forms the upper bound in 64 bits so a 32 bit index cannot wrap past it.
// That is what lets the access recompute 'base + displacement' in 64 bits where the index computed it in 32.
struct ValidatedBase
{
    IrOp buffer;
    IrOp base;
    int minOffset = 0;
    int maxOffset = 0;
};

// Individual constants are bounded to [-4095, 4095] upstream, but a chain of adds can accumulate past that
static const int64_t kMaxAccumulatedOffset = 1 << 24;

struct BufferOffsetFolder
{
    BufferOffsetFolder(IrBuilder& build, BufferOffsetTarget target)
        : build(build)
        , function(build.function)
        , target(target)
    {
    }

    IrBuilder& build;
    IrFunction& function;
    BufferOffsetTarget target;

    std::vector<ValidatedBase> validated;

    unsigned foldedCount = 0;
    unsigned uncoveredCount = 0;
    unsigned unencodableCount = 0;

    const ValidatedBase* findValidated(IrOp buffer, IrOp base) const
    {
        for (const ValidatedBase& entry : validated)
        {
            if (entry.buffer == buffer && entry.base == base)
                return &entry;
        }

        return nullptr;
    }

    void recordCheck(IrInst& inst)
    {
        if (OP_A(inst).kind != IrOpKind::Inst || OP_B(inst).kind != IrOpKind::Inst)
            return;

        if (OP_C(inst).kind != IrOpKind::Constant || OP_D(inst).kind != IrOpKind::Constant)
            return;

        int minOffset = function.intOp(OP_C(inst));
        int maxOffset = function.intOp(OP_D(inst));

        for (ValidatedBase& entry : validated)
        {
            if (entry.buffer == OP_A(inst) && entry.base == OP_B(inst))
            {
                // Both checks have passed, so the weaker end of each bound still holds
                if (minOffset < entry.minOffset)
                    entry.minOffset = minOffset;

                if (maxOffset > entry.maxOffset)
                    entry.maxOffset = maxOffset;

                return;
            }
        }

        validated.push_back(ValidatedBase{OP_A(inst), OP_B(inst), minOffset, maxOffset});
    }

    void tryFoldAccess(IrInst& inst)
    {
        BufferAccessShape shape;

        if (!getBufferAccessShape(inst.cmd, shape))
            return;

        if (OP_A(inst).kind != IrOpKind::Inst || OP_B(inst).kind != IrOpKind::Inst)
            return;

        // Nothing folds unless the index is a constant away from another value, and the tag lookup below is only
        // needed once one is found.
        IrInst* head = function.asInstOp(OP_B(inst));

        if (!head || head->cmd != IrCmd::ADD_INT)
            return;

        int dataOffset = getTagDataOffset(function.tagOp(getOp(inst, shape.tagSlot)));

        // Constant propagation rebases an access onto the previous check's operand, so an index can be a short chain
        // of them. Take the deepest base a check validated: the shared inner add only dies once every access hanging
        // off it has moved past it.
        IrOp current = OP_B(inst);
        int64_t accumulated = 0;

        IrOp foldBase = {};
        int foldOffset = 0;
        bool found = false;
        bool sawAdd = false;

        for (int depth = 0; depth < 8; depth++)
        {
            IrInst* add = function.asInstOp(current);

            if (!add || add->cmd != IrCmd::ADD_INT || OP_A(*add).kind != IrOpKind::Inst || OP_B(*add).kind != IrOpKind::Constant)
                break;

            sawAdd = true;
            accumulated += function.intOp(OP_B(*add));
            current = OP_A(*add);

            if (accumulated > kMaxAccumulatedOffset || accumulated < -kMaxAccumulatedOffset)
                break;

            // The lowering reads the index register as a zero extended 32 bit value, same as the check did
            if (producesDirtyHighRegisterBits(function.instOp(current).cmd))
                break;

            const ValidatedBase* entry = findValidated(OP_A(inst), current);

            if (!entry || accumulated < entry->minOffset || accumulated + shape.accessSize > entry->maxOffset)
                continue;

            if (!isEncodableDisplacement(target, int(accumulated) + dataOffset, shape.accessSize))
            {
                unencodableCount++;
                continue;
            }

            foldBase = current;
            foldOffset = int(accumulated);
            found = true;
        }

        if (!found)
        {
            if (sawAdd)
                uncoveredCount++;

            return;
        }

        // Set the displacement first: it can grow the operand vector, which invalidates references into it
        getOp(inst, shape.dispSlot) = build.constInt(foldOffset);

        replace(function, OP_B(inst), foldBase);

        foldedCount++;
    }
};

void optimizeBufferOffsets(IrBuilder& build, const std::vector<uint32_t>& sortedBlocks, BufferOffsetTarget target)
{
    if (!FFlag::LuauCodegenBufferOffsetFold)
        return;

    static const bool logging = getenv("LUAU_BUFFER_OFFSET_FOLD_LOG") != nullptr;

    IrFunction& function = build.function;
    BufferOffsetFolder folder(build, target);

    uint32_t previousBlockIdx = ~0u;

    for (uint32_t blockIdx : sortedBlocks)
    {
        IrBlock& block = function.blocks[blockIdx];

        if (block.kind == IrBlockKind::Dead)
            continue;

        // A check only speaks for the path that fell through it, so the set carries into the next block only when
        // that block's single predecessor is the one just scanned. Same chain rule that merged these checks.
        bool inherits = false;

        if (previousBlockIdx != ~0u && block.useCount == 1)
        {
            IrInst& terminator = function.instructions[function.blocks[previousBlockIdx].finish];

            if (terminator.cmd == IrCmd::JUMP && OP_A(terminator).kind == IrOpKind::Block && OP_A(terminator).index == blockIdx)
                inherits = true;
        }

        if (!inherits)
            folder.validated.clear();

        previousBlockIdx = blockIdx;

        // Inlining into the ExitSync block would disturb the operands listed in VM exit sync info argOps
        if (block.kind == IrBlockKind::ExitSync)
        {
            folder.validated.clear();
            continue;
        }

        for (uint32_t index = block.start; index <= block.finish; index++)
        {
            CODEGEN_ASSERT(index < function.instructions.size());
            IrInst& inst = function.instructions[index];

            if (inst.cmd == IrCmd::CHECK_BUFFER_LEN)
                folder.recordCheck(inst);
            else
                folder.tryFoldAccess(inst);
        }
    }

    if (logging && (folder.foldedCount != 0 || folder.uncoveredCount != 0 || folder.unencodableCount != 0))
    {
        fprintf(
            stderr,
            "[codegen-bufferoffset] proto '%s' (line %d): folded=%u uncovered=%u unencodable=%u\n",
            function.proto && function.proto->debugname ? getstr(function.proto->debugname) : "??",
            function.proto ? function.proto->linedefined : -1,
            folder.foldedCount,
            folder.uncoveredCount,
            folder.unencodableCount
        );
    }
}

} // namespace CodeGen
} // namespace Luau
