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

#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents how an element takes its place and lays out what it holds, every element having one.
    struct Box final
    {
        /// Whether the element flows among its siblings or stands over them.
        Position  Placement = Position::Flow;

        /// The axis the element lays its children out along.
        Direction Flow      = Direction::Column;

        /// `true` when children that overrun the axis start a new line of their own.
        Bool      Wrap      = false;

        /// Where the children sit across the axis.
        Align     Items     = Align::Stretch;

        /// How the room left along the axis is shared among the children.
        Justify   Content   = Justify::Start;

        /// The width of the element.
        Length    Width;

        /// The height of the element.
        Length    Height;

        /// The size along the parent's axis the element grows or shrinks from, or unset to start from its own.
        Length    Basis;

        /// How far an element standing over its parent sits from the parent's left edge, or unset to leave it free.
        Length    Left;

        /// How far an element standing over its parent sits from the parent's top edge, or unset to leave it free.
        Length    Top;

        /// How far an element standing over its parent sits from the parent's right edge, or unset to leave it free.
        Length    Right;

        /// How far an element standing over its parent sits from the parent's bottom edge, or unset to leave it free.
        Length    Bottom;

        /// The room kept free outside the element's box, between it and its siblings.
        Edges     Margin;

        /// The room kept free inside the element's box, between its edge and what it holds.
        Edges     Padding;

        /// The room between one child and the next, and between one line of them and the next, in pixels.
        Real32    Gap       = 0.0f;

        /// The share of the room left along the parent's axis the element grows into, where nought keeps its size.
        Real32    Grow      = 0.0f;

        /// How much of a row's overrun the element gives up, weighed by its size, where nought keeps its size.
        Real32    Shrink    = 1.0f;
    };
}