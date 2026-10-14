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

#include "Summon.hpp"
#include "Facet/Context.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/System/Pointer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Summon::Declare(Ref<Interface> Target)
    {
        if (Target.Find<Session>())
        {
            return;
        }

        Target.Provide<Session>();

        Pointer::Hook Gesture;
        Gesture.Offered = Pointer::Hook::OnOffered::Create<&Summon::OnOffered>();
        Target.GetCursor().AddHook(Move(Gesture));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Summon::OnOffered(Ref<Interface> Owner, ZyInput::Button Button, Bool Stopped)
    {
        // A right press no handler took opens the context menu of what it landed on.
        if (Button != ZyInput::Button::Right || Stopped)
        {
            return;
        }

        const Handle Over = Owner.GetOver();

        for (Handle Current = Over; Current.IsValid(); Current = Owner.GetParent(Current))
        {
            const ConstPtr<Context> Found = Owner.Get<Context>(Current);

            if (!Found)
            {
                continue;
            }

            // The menu hears what it was opened over, the nearest control under the pointer as a list's item.
            const Handle Menu    = Found->Menu;
            const Handle Subject = Owner.GetCursor().Nearest(Over, false);

            if (Owner.IsAlive(Menu) && Owner.GetDispatcher().IsUsable(Current))
            {
                Owner.Summon(Menu, Owner.GetPointer(), Subject.IsValid() ? Subject : Over);
            }
            return;
        }
    }
}