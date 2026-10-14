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

#include "Input.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Facet/Input/Keymap.hpp"
#include "UI.Core/Shortcut.hpp"
#include "UI.Markup/Markup.hpp"
#include "UI.Markup/Parser.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Input::Read(Ref<Markup>, Element Target, ConstRef<JsonValue> Node)
    {
        // A flag given makes the node a control, since asking for more of the pointer is what a control does.
        const ConstPtr<JsonValue> Hover     = Markup::GetField(Node, "Hover");
        const ConstPtr<JsonValue> AnyButton = Markup::GetField(Node, "AnyButton");
        const ConstPtr<JsonValue> Typing    = Markup::GetField(Node, "Typing");

        if (Hover && Hover->IsBool())
        {
            Target.Obtain<Control>().Hover = Hover->GetBool();
        }

        if (AnyButton && AnyButton->IsBool())
        {
            Target.Obtain<Control>().AnyButton = AnyButton->GetBool();
        }

        if (Typing && Typing->IsBool())
        {
            Target.Obtain<Control>().Typing = Typing->GetBool();
        }

        const ConstPtr<JsonValue> Shortcuts = Markup::GetField(Node, "Shortcuts");

        if (!Shortcuts)
        {
            return;
        }

        if (!Shortcuts->IsObject())
        {
            LOG_W("A node's 'Shortcuts' must map chords to actions, so it gives none");
            return;
        }

        Shortcuts->GetObject().ForEach([&Target](ConstRef<Str> Chord, ConstRef<JsonValue> Value)
        {
            Shortcut Entry;

            if (ReadShortcut(Chord, Value, Entry))
            {
                Target.Obtain<Keymap>().Entries.Append(Move(Entry));
            }
            else
            {
                LOG_W("The shortcut '{0}' is written badly, so the node is not given it", Text(Chord));
            }
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Input::ReadShortcut(Text Chord, ConstRef<JsonValue> Value, Ref<Shortcut> Output)
    {
        if (!Shortcut::Parse(Chord, Output.Key, Output.Modifiers))
        {
            return false;
        }

        // A node's own shortcut answers the pointer resting on it, unless it says otherwise.
        Output.Where = Shortcut::Scope::Hover;

        if (Value.IsString())
        {
            Output.Action = Identify(Value.GetString());
            return !Value.GetString().IsEmpty();
        }

        if (!Value.IsObject())
        {
            return false;
        }

        const Text Action = Markup::GetText(Value, "Action");

        const ConstPtr<JsonValue> Scope = Markup::GetField(Value, "Scope");

        if (Scope && !Parser::ParseEnum(* Scope, Output.Where))
        {
            return false;
        }

        Output.Action = Action.IsEmpty() ? 0 : Identify(Action);
        Output.Typing = Markup::GetBool(Value, "Typing", Output.Typing);
        Output.Blur   = Markup::GetBool(Value, "Blur", Output.Blur);
        Output.Repeat = Markup::GetBool(Value, "Repeat", Output.Repeat);
        return !Action.IsEmpty();
    }
}