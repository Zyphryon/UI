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
    /// \brief Represents every value of an element that is eased rather than changed at once.
    struct Appearance final
    {
        /// The colour the box is filled with.
        Color        Background  = Color::Transparent();

        /// The colour of the border.
        Color        Border      = Color::Transparent();

        /// The thickness of the border, in pixels.
        Real32       BorderWidth = 0.0f;

        /// The radius of the corners, in pixels.
        Real32       Radius      = 0.0f;

        /// How opaque the element and all it holds are, over nought through one.
        Real32       Opacity     = 1.0f;

        /// The colour of the text.
        Color        Tint        = Color::White();

        /// The colour of the outline around the text.
        Color        Stroke      = Color::Transparent();

        /// The thickness of the outline around the text, in pixels.
        Real32       StrokeWidth = 0.0f;

        /// The colour of a bar's filled share.
        Color        Fill        = Color::White();

        /// The colour of a scroll's thumb.
        Color        Thumb       = Color::White(0.3f);

        /// The gradient the box is filled with, over its background.
        UI::Gradient Gradient;

        /// The shadow cast behind the box.
        Halo         Shadow;

        /// The glow around the box.
        Halo         Glow;

        /// The glow just inside the box's edge.
        Halo         InnerGlow;

        /// How much larger the element and all it holds are drawn on each axis, about its middle.
        Vector2      Scale       = Vector2(1.0f, 1.0f);

        /// How far the element and all it holds are drawn turned, clockwise, in degrees.
        Real32       Rotate      = 0.0f;

        /// How far the element and all it holds are drawn moved, in pixels.
        Vector2      Translate;

        /// The colour the image a theme lays over the box is multiplied by.
        Color        ImageTint   = Color::White();
    };
}