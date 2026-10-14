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
#include "UI.Core/Facet/Visual/Picture.hpp"
#include "UI.Core/Paint/Canvas.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents an image, a picture fitted to the element's box, loaded from the content or the GPU.
    class Image final
    {
    public:

        /// \brief Creates an image.
        ///
        /// \param Parent The element to hold it.
        /// \param Source The picture to show.
        /// \param Style  The name of the style it wears, or empty for none.
        /// \return The image, showing the picture over its whole box.
        static Element Create(Element Parent, Retainer<ZyGraphic::Image> Source, Text Style = Text());

        /// \brief Creates an image showing a texture of the GPU as it stands, as the target a scene is rendered into.
        ///
        /// \param Parent  The element to hold it.
        /// \param Texture The texture, which the game keeps alive while it is shown.
        /// \param Layout  The shape of the texture, where an array is read at the image's layer.
        /// \param Style   The name of the style it wears, or empty for none.
        /// \return The image, showing the texture over its whole box, sized by its layout alone.
        static Element CreateView(
            Element                  Parent,
            ZyGraphic::Object        Texture,
            ZyGraphic::TextureLayout Layout = ZyGraphic::TextureLayout::Texture2D,
            Text                     Style  = Text());

        /// \brief Creates an image out of a node of a layout, showing the picture at its `Source`.
        ///
        /// \param Reader The markup reading the layout, which loads the picture and every one a binding names later.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the image is read from.
        /// \return The image, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Shows a picture of the content, in place of any texture of the GPU.
        ///
        /// \param Target The image whose picture is replaced.
        /// \param Source The picture, or none to show the texture given instead.
        static void SetSource(Element Target, Retainer<ZyGraphic::Image> Source);

        /// \brief Shows the picture at an address, loading only a changed one, as a bound `Source` does.
        ///
        /// \param Target  The image whose picture is replaced.
        /// \param Reader  The markup the picture is loaded through.
        /// \param Address The address of the picture, or empty to show none.
        static void SetAddress(Element Target, ConstRef<Markup> Reader, Text Address);

        /// \brief Shows a texture of the GPU as it stands while the image holds no picture of the content.
        ///
        /// \param Target  The image whose texture is replaced.
        /// \param Texture The texture, which the game keeps alive while it is shown, or nought for none.
        /// \param Layout  The shape of the texture, where an array is read at the image's layer.
        static void SetTexture(
            Element                  Target,
            ZyGraphic::Object        Texture,
            ZyGraphic::TextureLayout Layout = ZyGraphic::TextureLayout::Texture2D);

        /// \brief Picks the slice shown of a picture or a texture that is an array.
        ///
        /// \param Target The image whose slice is picked.
        /// \param Layer  The slice, where a value read as none shows the first.
        static void SetLayer(Element Target, UInt32 Layer);

        /// \brief Sets the part of the picture shown.
        ///
        /// \param Target The image whose region is set.
        /// \param Region The part, in texture coordinates, as a frame of a sheet.
        static void SetRegion(Element Target, Rect Region);

        /// \brief Sets the colour the picture is multiplied by.
        ///
        /// \param Target The image whose tint is set.
        /// \param Tint   The colour, white showing the picture as it is.
        static void SetTint(Element Target, Color Tint);

        /// \brief Sets how the picture is read between its texels.
        ///
        /// \param Target The image whose filter is set.
        /// \param Filter The filter, where pixel art and tiles want the nearest texel.
        static void SetFilter(Element Target, Canvas::Filter Filter);

        /// \brief Sets how the picture fills the image's box.
        ///
        /// \param Target The image whose fit is set.
        /// \param Mode   The way it fills, where only a picture of a known size keeps its shape.
        static void SetFit(Element Target, Fit Mode);

        /// \brief Sets whether the picture's alpha is ignored, as a target whose alpha holds data.
        ///
        /// \param Target The image whose alpha is ignored or heeded.
        /// \param Opaque The flag, `true` to show the colour alone, `false` to blend by the alpha.
        static void SetOpaque(Element Target, Bool Opaque);

        /// \brief Sets whether the picture is shown upside down.
        ///
        /// \param Target The image flipped or not.
        /// \param Flip   The flag, `true` to show its last row at the top, `false` to show it as it is.
        static void SetFlip(Element Target, Bool Flip);
    };
}