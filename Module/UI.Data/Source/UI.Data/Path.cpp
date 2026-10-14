// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Path.hpp"
#include "UI.Core/Handle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Path::Parse(Text Source, Ref<Path> Output)
    {
        // The steps are written straight into the path given, which keeps what it had room for.
        Ref<Path> Result = Output;
        Result.Steps.Clear();

        for (UInt Start = 0;;)
        {
            const SInt Open = StrFind(Source.Slice(Start), '[');

            // Past the last bracket, the rest is the name of the value, dots and all.
            if (Open < 0)
            {
                const Text Name = Source.Slice(Start);

                if (Name.IsEmpty())
                {
                    Result.Steps.Clear();
                    return false;
                }
                Result.Steps.Append(Identify(Name), Step::kLeaf);
                break;
            }

            const UInt Bracket = Start + static_cast<UInt>(Open);
            const SInt Close   = StrFind(Source.Slice(Bracket), ']');
            UInt32     Index   = 0;

            if (Open == 0 || Close < 0 || !ParseIndex(Source.Slice(Bracket + 1, static_cast<UInt>(Close) - 1), Index))
            {
                Result.Steps.Clear();
                return false;
            }

            Result.Steps.Append(Identify(Source.Slice(Start, static_cast<UInt>(Open))), Index);

            // A record is never a value of its own, so a dot and the name of one of its values must follow.
            Start = Bracket + static_cast<UInt>(Close) + 1;

            if (!StrConsume(Source, Start, '.'))
            {
                Result.Steps.Clear();
                return false;
            }
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Path::ParseIndex(Text Source, Ref<UInt32> Output)
    {
        UInt Cursor = 0;
        Output = StrExtractNumber<10, UInt32>(Source, Cursor);
        return Cursor > 0 && Cursor == Source.GetSize();
    }
}