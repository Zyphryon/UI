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

#include "Theme.hpp"
#include "UI.Core/Handle.hpp"
#include "Animation.hpp"
#include "Skin.hpp"
#include "Style.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Theme::Theme()
        : mVersion { 0 }
    {
    }

    Ref<Style> Theme::Define(Text Name, Text Base)
    {
        ++mVersion;

        const Digest Key   = Digest(Identify(Name));
        Ref<Style>   Entry = mStyles.FindOrInsert(Key);

        // A style defined before starts over, keeping nothing it gave.
        Entry      = Style();
        Entry.Base = Identify(Base);

        mNames.Assign(Key, Str(Name));
        return Entry;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Theme::Merge(Text Name, ConstRef<Style> Over)
    {
        ++mVersion;

        const Digest Key = Digest(Identify(Name));

        // A style none defined yet is merged over one giving nothing, which leaves it as it was given.
        mStyles.FindOrInsert(Key).Merge(Over);
        mNames.Assign(Key, Str(Name));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Theme::Adopt(Ref<Theme> Other)
    {
        ++mVersion;

        // A table moved from is left empty, so the other theme holds nothing after.
        mStyles     = Move(Other.mStyles);
        mAnimations = Move(Other.mAnimations);
        mNames      = Move(Other.mNames);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Theme::Resolve(UInt64 Name, State Current, Ref<Skin> Output) const
    {
        Skin Result;

        if (!Gather(Name, Current, Result, 0))
        {
            return false;
        }

        Output = Move(Result);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Style> Theme::Find(UInt64 Name) const
    {
        return mStyles.Find(Digest(Name));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ref<Animation> Theme::Animate(Text Name)
    {
        ++mVersion;

        const Digest   Key   = Digest(Identify(Name));
        Ref<Animation> Entry = mAnimations.FindOrInsert(Key);

        // An animation defined before starts over, keeping nothing it held.
        Entry = Animation();

        mNames.Assign(Key, Str(Name));
        return Entry;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Animation> Theme::FindAnimation(UInt64 Name) const
    {
        return mAnimations.Find(Digest(Name));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text Theme::FindSound(UInt64 Name, Cue Moment) const
    {
        // The nearest style naming the moment wins, even with an empty address, so a style can silence its base.
        ConstPtr<Style> Found = Name ? mStyles.Find(Digest(Name)) : nullptr;

        for (UInt32 Depth = 0; Found && Depth < kMaxDepth; ++Depth)
        {
            if (Found->Voiced.Test(ZyEnum::Cast(Moment)))
            {
                return Found->Sounds[ZyEnum::Cast(Moment)];
            }
            Found = Found->Base ? mStyles.Find(Digest(Found->Base)) : nullptr;
        }
        return Text();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Theme::Gather(UInt64 Name, State Current, Ref<Skin> Output, UInt32 Depth) const
    {
        const ConstPtr<Style> Found = mStyles.Find(Digest(Name));

        if (!Found || Depth >= kMaxDepth)
        {
            return false;
        }

        if (Found->Base)
        {
            Gather(Found->Base, Current, Output, Depth + 1);
        }
        Output.Merge(Found->Normal);

        // A value only a variant gives is still the style's, at its default, so leaving the state puts it back.
        for (ConstRef<Style::Variant> Entry : Found->Variants)
        {
            if (HasBit(Current, Entry.When))
            {
                Output.Merge(Entry.Wear);
            }
            else
            {
                Output.Mask |= Entry.Wear.Mask;
            }
        }
        return true;
    }
}