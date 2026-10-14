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

#include "Datum.hpp"
#include "Path.hpp"
#include "Piece.hpp"
#include "UI.Core/Element.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Defines the code that shows a value of a model on an element, through its widget's own setter.
    using Writer = Delegate<void(Element Target, ConstRef<Datum> Value)>;

    /// \brief Defines the code that reads back what the user made of an element, for a binding going both ways.
    using Reader = Delegate<Datum(Element Target)>;

    /// \brief Represents one field of an element bound to a model.
    struct Link final
    {
        /// The pieces of its text, literal text and values, in the order they are written out.
        Sequence<Piece> Pieces;

        /// The code showing the value on the element.
        Writer          Write;

        /// The code reading the element's value back, or empty for a binding that only shows.
        Reader          Read;

        /// The stamp of the values it last showed, so an unchanged value is never shown again.
        UInt32          Seen = 0;

        /// The lexicon key of its text, for a field written `@Key`, or empty for one written out in place.
        Str             Phrase;

        /// The path of the record it was made in, as written, so a translated text reads from the same record.
        Str             Scope;

        /// The steps its scope gives the paths it reads, moved with them when its row is handed to another record.
        Path            Within;
    };
}