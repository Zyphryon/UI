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
#include "UI.Core/Interface.hpp"
#include "UI.Core/Paint/Canvas.hpp"
#include "UI.Core/System/Painter.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the colour a swatch shows, over a board of light and dark squares so transparency shows.
    struct Checker final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::Paint;

        /// The colour shown, kept apart from the look a style animates so a transition never puts an old one back.
        Color Value = Color::White();
    };

    /// \brief Represents a swatch, a box filled with a colour over a board of squares, so its transparency shows.
    class Swatch final
    {
    public:

        /// \brief Creates a swatch.
        ///
        /// \param Parent The element to hold it.
        /// \param Value  The colour it shows.
        /// \param Style  The name of the style it wears.
        /// \return The swatch, which reports nothing and stops no pointer of its own.
        static Element Create(Element Parent, Color Value = Color::White(), Text Style = "Swatch");

        /// \brief Creates a swatch from a node, showing its `Value`, a colour as `#RRGGBBAA` or a binding.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the swatch is read from.
        /// \return The swatch, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Installs the drawing of the board every element holding a \ref Checker shows, once for an interface.
        ///
        /// \param Target The interface whose painter draws the boards.
        static void Declare(Ref<Interface> Target);

        /// \brief Changes the colour a swatch shows.
        ///
        /// \param Target The swatch to change.
        /// \param Value  The colour it shows from now on.
        static void SetValue(Element Target, Color Value);

        /// \brief Gets the colour a swatch shows.
        ///
        /// \param Target The swatch to read.
        /// \return The colour, its fill.
        static Color GetValue(Element Target);

    private:

        /// \brief Draws the board of an element holding a \ref Checker and its colour over it, under its fill.
        ///
        /// \param Owner   The interface the element stands in, whose facets give the colour.
        /// \param Board   The canvas to draw through.
        /// \param Element The element drawn, whose box the board fills.
        static void OnPaint(Ref<Interface> Owner, Ref<Canvas> Board, ConstRef<Painter::Brush> Element);
    };
}