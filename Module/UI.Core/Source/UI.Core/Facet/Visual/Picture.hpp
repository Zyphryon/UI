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

#include "UI.Core/Paint/Canvas.hpp"
#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Specifies how a picture fills the box of the element showing it.
    enum class Fit : UInt8
    {
        Stretch,     ///< Stretched over the whole box, whatever its shape.
        Contain,     ///< As large as fits whole within the box, its shape kept, in the middle of the box.
    };

    /// \brief Represents the image an element shows over its box, loaded from the content or any texture of the GPU.
    struct Picture final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks       = Dirty::Measure | Dirty::Paint;

        /// `true`, so the pointer stops at an element holding it.
        static constexpr Bool  kInteractive = true;

        /// The image shown over the element's box, which wins over \ref Texture while it holds one.
        Retainer<ZyGraphic::Image> Source;

        /// The texture shown while no \ref Source is held, as a render target, which the game keeps alive.
        ZyGraphic::Object          Texture = 0;

        /// The shape of \ref Texture, where an array is read at \ref Layer.
        ZyGraphic::TextureLayout   Layout  = ZyGraphic::TextureLayout::Texture2D;

        /// The part of the image shown, in texture coordinates.
        Rect                       Region  = Rect::One();

        /// The border of the image kept unstretched on each side, in pixels of the image, which slices it in nine.
        Edges                      Slice;

        /// The colour the image is multiplied by.
        Color                      Tint    = Color::White();

        /// The slice shown of an image or a texture that is an array.
        UInt16                     Layer   = 0;

        /// How the image is read between its texels, where pixel art and tiles want the nearest.
        Canvas::Filter             Filter  = Canvas::Filter::Linear;

        /// How the image fills the box, where only an image of a known size keeps its shape.
        UI::Fit                    Fit     = UI::Fit::Stretch;

        /// `true` to show the image's colour alone with its alpha taken as whole, as a target whose alpha holds data.
        Bool                       Opaque  = false;

        /// `true` to show the image upside down, its last row at the top.
        Bool                       Flip    = false;
    };
}