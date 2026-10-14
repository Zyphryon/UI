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

#include "Memory.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Memory::GetKey(ConstRef<Markup> Reader, ConstRef<JsonValue> Node)
    {
        const Text Name  = Markup::GetText(Node, "Name");
        const Text Scope = Reader.GetScope();

        if (Name.IsEmpty())
        {
            return Str();
        }

        // A template made once a record, as an inspector's section, is told apart by the record it reads.
        return Scope.IsEmpty() ? Str(Name) : Str::Join(Scope, '/', Name);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Memory::Hold(Element Target, Text Key)
    {
        if (!Target.IsAlive())
        {
            return;
        }

        if (Key.IsEmpty())
        {
            if (Target.Has<Recall>())
            {
                Target.Remove<Recall>();
            }
            return;
        }

        const UInt64            Name  = Identify(Key);
        const ConstPtr<Vector2> Found = Supply(Target.GetTree()).Scrolled.Find(Digest(Name));
        const ConstPtr<Scroll>  Roll  = Target.Get<Scroll>();
        const Vector2           Now   = Roll ? Vector2(Roll->OffsetX, Roll->Offset) : Vector2();

        Ref<Recall> Kept = Target.Obtain<Recall>();
        Kept.Key       = Name;
        Kept.Seen      = Now;
        Kept.Wanted    = Found ? * Found : Now;
        Kept.Restoring = Roll && Found && * Found != Now;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Memory::SetOpened(Element Target, Bool Opened)
    {
        if (const ConstPtr<Recall> Kept = Target.IsAlive() ? Target.Get<Recall>() : nullptr)
        {
            const UInt64 Name = Kept->Key;
            Supply(Target.GetTree()).Opened.FindOrInsert(Digest(Name)) = Opened;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Memory::FindOpened(Element Target, Ref<Bool> Output)
    {
        const ConstPtr<Recall> Kept  = Target.IsAlive() ? Target.Get<Recall>() : nullptr;
        const ConstPtr<Bool>   Found = Kept ? Supply(Target.GetTree()).Opened.Find(Digest(Kept->Key)) : nullptr;

        if (!Found)
        {
            return false;
        }

        Output = * Found;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Memory::Track(Element Target)
    {
        const Ptr<Recall>      Kept = Target.Get<Recall>();
        const ConstPtr<Scroll> Roll = Target.Get<Scroll>();

        if (!Kept || !Roll)
        {
            return false;
        }

        const Vector2 Now(Roll->OffsetX, Roll->Offset);

        // A move the user makes while it waits wins, and is what it remembers from then on.
        if (Kept->Restoring && Now == Kept->Seen)
        {
            const Real32 LimitY = Max(Roll->Overflow  - Roll->View.GetY(), 0.0f);
            const Real32 LimitX = Max(Roll->OverflowX - Roll->View.GetX(), 0.0f);

            // Brought back only once what it holds reaches that far, so the layout never cuts the scroll short.
            if (Kept->Wanted.GetY() > LimitY + 0.5f || Kept->Wanted.GetX() > LimitX + 0.5f)
            {
                return false;
            }

            const Vector2 Wanted = Kept->Wanted;
            Kept->Restoring = false;

            const Ptr<Scroll> Moved = Target.Edit<Scroll>();
            Moved->Offset  = Wanted.GetY();
            Moved->OffsetX = Wanted.GetX();
            return true;
        }

        Kept->Restoring = false;

        // The memory is written only when the scroll moved, so a still list costs one lookup.
        if (Ref<Vector2> Slot = Supply(Target.GetTree()).Scrolled.FindOrInsert(Digest(Kept->Key)); Slot != Now)
        {
            Slot = Now;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ref<Memory::Store> Memory::Supply(Ref<Tree> Owner)
    {
        if (const Ptr<Store> Found = Owner.Find<Store>())
        {
            return * Found;
        }
        return Owner.Provide<Store>();
    }
}