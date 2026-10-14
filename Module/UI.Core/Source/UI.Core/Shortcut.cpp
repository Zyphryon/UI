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

#include "Shortcut.hpp"
#include "Handle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Shortcut::Shortcut(Text Chord, Text Action)
        : Action { Action.IsEmpty() ? 0 : Identify(Action) }
    {
        if (!Parse(Chord, Key, Modifiers))
        {
            LOG_W("The shortcut '{0}' names no key, so no key runs it", Chord);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Shortcut::Parse(Text Chord, Ref<ZyInput::Key> Key, Ref<ZyInput::Modifier> Modifiers)
    {
        // The key follows the last plus sign, and every part before it names a modifier.
        const Bool Joined = StrFindLast(Chord, '+') != -1;
        const Text Name   = Joined ? StrAfterLast(Chord, '+') : Chord;
        Bool       Read   = !Name.IsEmpty();

        ZyInput::Modifier Held = ZyInput::Modifier::None;

        if (Joined)
        {
            StrSplit(StrBeforeLast(Chord, '+'), '+', [&Held, &Read](Text Part)
            {
                if (StrEqualCaseInsensitive(Part, "Ctrl") || StrEqualCaseInsensitive(Part, "Control"))
                {
                    Held |= ZyInput::Modifier::Control;
                }
                else if (StrEqualCaseInsensitive(Part, "Shift"))
                {
                    Held |= ZyInput::Modifier::Shift;
                }
                else if (StrEqualCaseInsensitive(Part, "Alt"))
                {
                    Held |= ZyInput::Modifier::Alt;
                }
                else
                {
                    Read = false;
                }
                return Read;
            });
        }

        // A digit alone names the key above the letters; any other name must read back as the key it names.
        ZyInput::Key Found = ZyInput::Key::Unknown;

        if (Name.GetSize() == 1 && StrIsDigit(Name[0]))
        {
            Found = static_cast<ZyInput::Key>(ZyEnum::Cast(ZyInput::Key::Alpha0) + (Name[0] - '0'));
        }
        else if (Read)
        {
            Found = ZyEnum::Cast<ZyInput::Key>(Name, ZyInput::Key::Unknown);
        }

        if (!Read || Found == ZyInput::Key::Unknown)
        {
            return false;
        }

        Key       = Found;
        Modifiers = Held;
        return true;
    }
}