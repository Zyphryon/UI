// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "UI.Core/Element.hpp"
#include "UI.Core/Shortcut.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Provides the fields a node may give to ask more of the pointer and keyboard, as hover and shortcuts.
    class Input final
    {
    public:

        /// \brief Reads the `Hover`, `AnyButton` and `Typing` flags and the `Shortcuts` mapping chords to actions.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Target The element made from the node.
        /// \param Node   The layout node the fields are read from.
        static void Read(Ref<Markup> Reader, Element Target, ConstRef<JsonValue> Node);

    private:

        /// \brief Reads one shortcut a node gives, by its chord and what it runs.
        ///
        /// \param Chord  The chord, as \ref Shortcut::Parse reads it.
        /// \param Value  The name of the action it runs, or an object giving it and how it runs.
        /// \param Output The variable receiving the shortcut.
        /// \return `true` if it was read, `false` when the chord or the value is written badly.
        static Bool ReadShortcut(Text Chord, ConstRef<JsonValue> Value, Ref<Shortcut> Output);
    };
}