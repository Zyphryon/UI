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
#include "UI.Core/Event.hpp"
#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Provides what every widget of the library is made with.
    class Widget final
    {
    public:

        /// \brief Gives an element a control running a behaviour, keeping the control it already has.
        ///
        /// \param Target    The element to make a control of.
        /// \param Behaviour The behaviour that answers its events, or empty for none.
        /// \param Focusable The flag, `true` to let a press move the keyboard to it, `false` otherwise.
        static void Govern(Element Target, Handler Behaviour, Bool Focusable);

        /// \brief Gets the name of the style a widget's part wears, the widget's style followed by the part's.
        ///
        /// \param Style The style of the widget, as `Button`.
        /// \param Part  The name of the part, as `Label`.
        /// \return The name of the part's style, as `Button.Label`.
        static Str Part(Text Style, Text Part);

        /// \brief Gets a child of an element by its place among the others.
        ///
        /// \param Parent The element whose children are walked.
        /// \param Index  The place of the child, counted from the first.
        /// \return The child, or one naming nothing past the last.
        static Element GetChild(Element Parent, UInt32 Index);

        /// \brief Counts the children of an element.
        ///
        /// \param Parent The element whose children are counted.
        /// \return The number of children it holds.
        static UInt32 GetChildCount(Element Parent);

        /// \brief Gets the state of a part picked among its siblings, declaring it the first time.
        ///
        /// \return The state a theme dresses a picked item, tab or entry by.
        static State Selected();

        /// \brief Gets the state of a part turned on, declaring it the first time.
        ///
        /// \return The state a theme dresses a checked box or an open expander by.
        static State Checked();
    };
}