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
#include "UI.Core/Types.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents a label, a line or more of text.
    class Label final
    {
    public:

        /// \brief Creates a label.
        ///
        /// \param Parent  The element to hold it.
        /// \param Content The text it shows.
        /// \param Style   The name of the style it wears.
        /// \return The label, showing the text in its style's font.
        static Element Create(Element Parent, Text Content, Text Style = "Label");

        /// \brief Creates a label from a layout node, showing its `Text`, ellipsized when `Overflow` is `"Ellipsis"`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the label is read from.
        /// \return The label, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Changes the text a label shows.
        ///
        /// \param Target  The label whose text changes.
        /// \param Content The text to show.
        static void SetText(Element Target, Text Content);

        /// \brief Sets what a line of a label too long for its box does; an ellipsis never wraps.
        ///
        /// \param Target The label whose lines are cut or wrapped.
        /// \param Mode   The way a line too long for the box behaves.
        static void SetOverflow(Element Target, Overflow Mode);
    };
}