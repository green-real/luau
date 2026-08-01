// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "Luau/IrDetailReport.h"

#include "Luau/IrData.h"
#include "Luau/IrDump.h"
#include "Luau/IrUtils.h"
#include "Luau/LoweringStats.h"

#include <algorithm>
#include <string>
#include <vector>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace Luau
{
namespace CodeGen
{

namespace
{

struct Row
{
    const char* name = nullptr;
    uint64_t count = 0;
};

void appendLine(std::string& out, const char* format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    out.append(buffer);
}

double share(uint64_t part, uint64_t whole)
{
    return whole == 0 ? 0.0 : 100.0 * double(part) / double(whole);
}

uint64_t classTotal(const IrDetailStats& detail, size_t blockClass)
{
    uint64_t total = 0;

    for (size_t cmd = 0; cmd < kIrCmdSlots; cmd++)
        total += detail.cmdByClass[cmd * IrBlockClass_Count + blockClass];

    return total;
}

uint64_t cmdCount(const IrDetailStats& detail, IrCmd cmd, size_t blockClass)
{
    return detail.cmdByClass[size_t(uint8_t(cmd)) * IrBlockClass_Count + blockClass];
}

uint64_t cmdTotal(const IrDetailStats& detail, IrCmd cmd)
{
    uint64_t total = 0;

    for (size_t c = 0; c < IrBlockClass_Count; c++)
        total += cmdCount(detail, cmd, c);

    return total;
}

uint64_t edge(const IrDetailStats& detail, IrCmd def, IrCmd use)
{
    return detail.defUse[size_t(uint8_t(def)) * kIrCmdSlots + size_t(uint8_t(use))];
}

bool startsWith(const char* text, const char* prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

// A guard: a test whose only job is to prove an assumption still holds, so the work it protects can stay on the
// fast path. It computes nothing the program asked for.
bool isGuardCmd(IrCmd cmd)
{
    return startsWith(getCmdName(cmd), "CHECK_");
}

// A conversion between representations. These are the tax a representation choice charges at its boundaries.
bool isConversionCmd(IrCmd cmd)
{
    switch (cmd)
    {
    case IrCmd::INT_TO_NUM:
    case IrCmd::INT64_TO_NUM:
    case IrCmd::UINT_TO_NUM:
    case IrCmd::UINT_TO_FLOAT:
    case IrCmd::NUM_TO_INT:
    case IrCmd::NUM_TO_INT64:
    case IrCmd::NUM_TO_UINT:
    case IrCmd::FLOAT_TO_NUM:
    case IrCmd::NUM_TO_FLOAT:
    case IrCmd::FLOAT_TO_VEC:
    case IrCmd::TRUNCATE_UINT:
        return true;
    default:
        return false;
    }
}

bool isMemoryCmd(IrCmd cmd)
{
    const char* name = getCmdName(cmd);
    return startsWith(name, "LOAD_") || startsWith(name, "STORE_") || startsWith(name, "BUFFER_");
}

void appendTopCommands(std::string& out, const IrDetailStats& detail, size_t blockClass, size_t limit)
{
    std::vector<Row> rows;

    for (size_t cmd = 0; cmd < kIrCmdSlots; cmd++)
    {
        const uint64_t count = detail.cmdByClass[cmd * IrBlockClass_Count + blockClass];

        if (count != 0)
            rows.push_back({getCmdName(IrCmd(cmd)), count});
    }

    std::sort(
        rows.begin(),
        rows.end(),
        [](const Row& l, const Row& r)
        {
            return l.count > r.count;
        }
    );

    const uint64_t total = classTotal(detail, blockClass);

    for (size_t i = 0; i < rows.size() && i < limit; i++)
        appendLine(out, "  %-28s %10llu  %5.2f%%\n", rows[i].name, (unsigned long long)rows[i].count, share(rows[i].count, total));
}

} // namespace

std::string reportIrDetail(const IrDetailStats& detail)
{
    std::string out;

    if (detail.empty())
        return "ir detail: not collected\n";

    const uint64_t fast = classTotal(detail, IrBlockClass_Fast);
    const uint64_t fallback = classTotal(detail, IrBlockClass_Fallback);
    const uint64_t exitSync = classTotal(detail, IrBlockClass_ExitSync);
    const uint64_t all = fast + fallback + exitSync;

    appendLine(out, "== IR totals ==\n");
    appendLine(out, "  instructions            %10llu\n", (unsigned long long)all);
    appendLine(out, "    fast path             %10llu  %5.2f%%\n", (unsigned long long)fast, share(fast, all));
    appendLine(out, "    fallback              %10llu  %5.2f%%\n", (unsigned long long)fallback, share(fallback, all));
    appendLine(out, "    exit sync             %10llu  %5.2f%%\n", (unsigned long long)exitSync, share(exitSync, all));
    appendLine(
        out,
        "  blocks                  %10llu fast, %llu fallback, %llu exit sync\n",
        (unsigned long long)detail.blocksByClass[IrBlockClass_Fast],
        (unsigned long long)detail.blocksByClass[IrBlockClass_Fallback],
        (unsigned long long)detail.blocksByClass[IrBlockClass_ExitSync]
    );

    // Deopt-path volume is output that exists only so the program can stop running natively. It is not wasted in the
    // sense of being removable, but it is a cost the fast path imposes and is worth sizing against it.
    appendLine(out, "\n== Cost of leaving native ==\n");
    appendLine(
        out,
        "  deopt-path instructions %10llu  %5.2f%% of all IR\n",
        (unsigned long long)(fallback + exitSync),
        share(fallback + exitSync, all)
    );

    uint64_t guardFast = 0;
    uint64_t guardAll = 0;
    std::vector<Row> guards;

    // Only slots with a count name a command that exists: the array is sized to the whole uint8_t range so it never
    // depends on which command happens to be declared last, and naming an unused slot would read past the enum.
    for (size_t cmd = 0; cmd < kIrCmdSlots; cmd++)
    {
        const uint64_t total = cmdTotal(detail, IrCmd(cmd));

        if (total == 0 || !isGuardCmd(IrCmd(cmd)))
            continue;

        guardAll += total;
        guardFast += cmdCount(detail, IrCmd(cmd), IrBlockClass_Fast);
        guards.push_back({getCmdName(IrCmd(cmd)), total});
    }

    std::sort(
        guards.begin(),
        guards.end(),
        [](const Row& l, const Row& r)
        {
            return l.count > r.count;
        }
    );

    appendLine(out, "\n== Guards ==\n");
    appendLine(out, "  guard instructions      %10llu  %5.2f%% of all IR\n", (unsigned long long)guardAll, share(guardAll, all));
    appendLine(out, "  of which on fast path   %10llu  %5.2f%% of fast path\n", (unsigned long long)guardFast, share(guardFast, fast));

    for (const Row& row : guards)
        appendLine(out, "  %-28s %10llu\n", row.name, (unsigned long long)row.count);

    // Conversions are the representation tax. A round trip is the part that is unambiguously wasted: a value is
    // moved into another representation and immediately moved back, so the pair computes nothing.
    uint64_t conversionAll = 0;
    std::vector<Row> conversions;

    for (size_t cmd = 0; cmd < kIrCmdSlots; cmd++)
    {
        const uint64_t total = cmdTotal(detail, IrCmd(cmd));

        if (total == 0 || !isConversionCmd(IrCmd(cmd)))
            continue;

        conversionAll += total;
        conversions.push_back({getCmdName(IrCmd(cmd)), total});
    }

    std::sort(
        conversions.begin(),
        conversions.end(),
        [](const Row& l, const Row& r)
        {
            return l.count > r.count;
        }
    );

    appendLine(out, "\n== Representation conversions ==\n");
    appendLine(out, "  conversion instructions %10llu  %5.2f%% of all IR\n", (unsigned long long)conversionAll, share(conversionAll, all));

    for (const Row& row : conversions)
        appendLine(out, "  %-28s %10llu\n", row.name, (unsigned long long)row.count);

    static const struct
    {
        IrCmd out;
        IrCmd back;
    } kRoundTrips[] = {
        {IrCmd::NUM_TO_INT, IrCmd::INT_TO_NUM},
        {IrCmd::INT_TO_NUM, IrCmd::NUM_TO_INT},
        {IrCmd::NUM_TO_UINT, IrCmd::UINT_TO_NUM},
        {IrCmd::UINT_TO_NUM, IrCmd::NUM_TO_UINT},
        {IrCmd::NUM_TO_FLOAT, IrCmd::FLOAT_TO_NUM},
        {IrCmd::FLOAT_TO_NUM, IrCmd::NUM_TO_FLOAT},
        {IrCmd::NUM_TO_INT64, IrCmd::INT64_TO_NUM},
        {IrCmd::INT64_TO_NUM, IrCmd::NUM_TO_INT64},
    };

    appendLine(out, "\n  round trips (a value converted and immediately converted back)\n");

    uint64_t roundTripTotal = 0;

    for (const auto& pair : kRoundTrips)
    {
        const uint64_t count = edge(detail, pair.out, pair.back);

        if (count == 0)
            continue;

        roundTripTotal += count;
        appendLine(out, "  %-16s -> %-16s %10llu\n", getCmdName(pair.out), getCmdName(pair.back), (unsigned long long)count);
    }

    appendLine(
        out,
        "  %-36s %10llu  %5.2f%% of conversions\n",
        "total",
        (unsigned long long)roundTripTotal,
        share(roundTripTotal, conversionAll)
    );

    appendLine(out, "\n== Representation of results ==\n");

    for (size_t v = 0; v < kIrValueKindSlots; v++)
    {
        uint64_t total = 0;

        for (size_t c = 0; c < IrBlockClass_Count; c++)
            total += detail.valueKindByClass[v * IrBlockClass_Count + c];

        if (total == 0)
            continue;

        appendLine(
            out,
            "  %-12s %10llu total, %10llu fast  %5.2f%% of fast results\n",
            getValueKindName(IrValueKind(v)),
            (unsigned long long)total,
            (unsigned long long)detail.valueKindByClass[v * IrBlockClass_Count + IrBlockClass_Fast],
            share(detail.valueKindByClass[v * IrBlockClass_Count + IrBlockClass_Fast], fast)
        );
    }

    uint64_t memoryAll = 0;

    for (size_t cmd = 0; cmd < kIrCmdSlots; cmd++)
    {
        const uint64_t total = cmdTotal(detail, IrCmd(cmd));

        if (total != 0 && isMemoryCmd(IrCmd(cmd)))
            memoryAll += total;
    }

    appendLine(out, "\n== Memory traffic ==\n");
    appendLine(out, "  load/store/buffer ops   %10llu  %5.2f%% of all IR\n", (unsigned long long)memoryAll, share(memoryAll, all));

    // A result nothing reads is work the backend still lowers. A result read exactly once cannot be shared and is a
    // rematerialization candidate rather than a spill candidate.
    uint64_t results = 0;

    for (uint32_t count : detail.useCountBuckets)
        results += count;

    appendLine(out, "\n== Value reuse ==\n");
    appendLine(out, "  results produced        %10llu\n", (unsigned long long)results);

    static const char* const kBucketNames[] = {"unused", "1 use", "2 uses", "3 uses", "4-7 uses", "8+ uses"};

    for (size_t b = 0; b < detail.useCountBuckets.size() && b < 6; b++)
        appendLine(
            out,
            "  %-12s %10llu  %5.2f%%\n",
            kBucketNames[b],
            (unsigned long long)detail.useCountBuckets[b],
            share(detail.useCountBuckets[b], results)
        );

    appendLine(out, "\n== Fast path, most frequent commands ==\n");
    appendTopCommands(out, detail, IrBlockClass_Fast, 25);

    if (exitSync != 0)
    {
        appendLine(out, "\n== Exit sync, most frequent commands ==\n");
        appendTopCommands(out, detail, IrBlockClass_ExitSync, 10);
    }

    if (fallback != 0)
    {
        appendLine(out, "\n== Fallback, most frequent commands ==\n");
        appendTopCommands(out, detail, IrBlockClass_Fallback, 10);
    }

    // The def-use edges say what a value was computed for, which adjacency in a printed dump cannot: scheduling is
    // not dataflow, and a dump does not say which operand refers to which result.
    std::vector<std::pair<uint64_t, std::string>> edges;

    for (size_t def = 0; def < kIrCmdSlots; def++)
    {
        for (size_t use = 0; use < kIrCmdSlots; use++)
        {
            const uint64_t count = detail.defUse[def * kIrCmdSlots + use];

            if (count == 0)
                continue;

            std::string label = getCmdName(IrCmd(def));
            label += " -> ";
            label += getCmdName(IrCmd(use));
            edges.push_back({count, label});
        }
    }

    std::sort(
        edges.begin(),
        edges.end(),
        [](const std::pair<uint64_t, std::string>& l, const std::pair<uint64_t, std::string>& r)
        {
            return l.first > r.first;
        }
    );

    uint64_t edgeTotal = 0;

    for (const auto& e : edges)
        edgeTotal += e.first;

    appendLine(out, "\n== What values are computed for, most frequent def-use edges ==\n");
    appendLine(out, "  dataflow edges          %10llu\n", (unsigned long long)edgeTotal);

    for (size_t i = 0; i < edges.size() && i < 30; i++)
        appendLine(out, "  %-44s %10llu  %5.2f%%\n", edges[i].second.c_str(), (unsigned long long)edges[i].first, share(edges[i].first, edgeTotal));

    return out;
}

} // namespace CodeGen
} // namespace Luau
