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

#include "Transition.hpp"
#include "UI.Core/Types.hpp"
#include <Zyphryon.Graphic/Image.hpp>
#include <Zyphryon.Render/Font/Font.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents a set of visual values a theme gives an element, only the ones it names being given.
    struct Skin final
    {
        /// \brief Specifies one value a skin may give, one bit each in its mask.
        enum class Field : UInt64
        {
            None        = 0,           ///< No value.
            Background  = 1ull << 0,   ///< The colour the box is filled with.
            Border      = 1ull << 1,   ///< The colour of the border.
            BorderWidth = 1ull << 2,   ///< The thickness of the border.
            Radius      = 1ull << 3,   ///< The radius of the corners.
            Opacity     = 1ull << 4,   ///< How opaque the element and all it holds are.
            Tint        = 1ull << 5,   ///< The colour of the text.
            Size        = 1ull << 6,   ///< The size the text is set at.
            Font        = 1ull << 7,   ///< The typeface of the text.
            Stroke      = 1ull << 8,   ///< The colour of the outline around the text.
            StrokeWidth = 1ull << 9,   ///< The thickness of the outline around the text.
            Padding     = 1ull << 10,  ///< The room kept inside the box.
            Width       = 1ull << 11,  ///< The width of the element.
            Height      = 1ull << 12,  ///< The height of the element.
            Fill        = 1ull << 13,  ///< The colour of a bar's filled share.
            Thumb       = 1ull << 14,  ///< The colour of a scroll's thumb.
            Alignment   = 1ull << 15,  ///< Where each line of the text sits.
            Gap         = 1ull << 16,  ///< The room between the children.
            Gradient    = 1ull << 17,  ///< The gradient the box is filled with, over its background.
            Shadow      = 1ull << 18,  ///< The shadow cast behind the box.
            Glow        = 1ull << 19,  ///< The glow around the box.
            InnerGlow   = 1ull << 20,  ///< The glow just inside the box's edge.
            Scale       = 1ull << 21,  ///< How much larger the element is drawn, about its middle.
            Rotate      = 1ull << 22,  ///< How far the element is drawn turned, about its middle.
            Translate   = 1ull << 23,  ///< How far the element is drawn moved.
            Transition  = 1ull << 24,  ///< How the element moves to the values a new state gives.
            Animation   = 1ull << 25,  ///< The animation the element plays, looping, while it wears the skin.
            Enter       = 1ull << 26,  ///< The animation the element plays once when it appears.
            Exit        = 1ull << 27,  ///< The animation the element plays once when it is hidden, before it goes.
            Image       = 1ull << 28,  ///< The image laid over the box, between its fill and its picture.
            Region      = 1ull << 29,  ///< The part of that image shown.
            Slice       = 1ull << 30,  ///< The border of that image kept unstretched, which slices it in nine.
            ImageTint   = 1ull << 31,  ///< The colour that image is multiplied by.
        };
        ZY_DEFINE_BITWISE_FRIEND_ENUM(Field)

        /// The values the skin gives, one bit each.
        Field                      Mask        = Field::None;

        /// The colour the box is filled with.
        Color                      Background  = Color::Transparent();

        /// The colour of the border.
        Color                      Border      = Color::Transparent();

        /// The thickness of the border, in pixels.
        Real32                     BorderWidth = 0.0f;

        /// The radius of the corners, in pixels.
        Real32                     Radius      = 0.0f;

        /// How opaque the element and all it holds are, over nought through one.
        Real32                     Opacity     = 1.0f;

        /// The colour of the text.
        Color                      Tint        = Color::White();

        /// The size the text is set at, in pixels.
        Real32                     Size        = 16.0f;

        /// The typeface of the text.
        Retainer<ZyRender::Font>   Font;

        /// The colour of the outline around the text.
        Color                      Stroke      = Color::Transparent();

        /// The thickness of the outline around the text, in pixels.
        Real32                     StrokeWidth = 0.0f;

        /// The room kept inside the box.
        Edges                      Padding;

        /// The width of the element.
        Length                     Width;

        /// The height of the element.
        Length                     Height;

        /// The colour of a bar's filled share.
        Color                      Fill        = Color::White();

        /// The colour of a scroll's thumb.
        Color                      Thumb       = Color::White(0.3f);

        /// Where each line of the text sits.
        Align                      Alignment   = Align::Start;

        /// The room between the children, in pixels.
        Real32                     Gap         = 0.0f;

        /// The gradient the box is filled with, over its background.
        UI::Gradient               Gradient;

        /// The shadow cast behind the box.
        Halo                       Shadow;

        /// The glow around the box, a shadow that is not moved.
        Halo                       Glow;

        /// The glow just inside the box's edge, whose offset and spread are not read.
        Halo                       InnerGlow;

        /// How much larger the element and all it holds are drawn on each axis, about its middle.
        Vector2                    Scale       = Vector2(1.0f, 1.0f);

        /// How far the element and all it holds are drawn turned, clockwise, in degrees.
        Real32                     Rotate      = 0.0f;

        /// How far the element and all it holds are drawn moved, in pixels.
        Vector2                    Translate   = Vector2(0.0f, 0.0f);

        /// How the element moves to the values a new state gives.
        UI::Transition             Transition;

        /// The identifier of the animation the element plays, looping, while it wears the skin, or zero for none.
        UInt64                     Animation   = 0;

        /// The identifier of the animation the element plays once when it appears, or zero for none.
        UInt64                     Enter       = 0;

        /// The identifier of the animation the element plays once when it is hidden, or zero for none.
        UInt64                     Exit        = 0;

        /// The image laid over the box, between its fill and its picture, or none.
        Retainer<ZyGraphic::Image> Image;

        /// The part of that image shown, in texture coordinates.
        Rect                       Region      = Rect::One();

        /// The border of that image kept unstretched on each side, in pixels of the image, or none to stretch it.
        Edges                      Slice;

        /// The colour that image is multiplied by.
        Color                      ImageTint   = Color::White();

        /// \brief Checks whether the skin gives a value.
        ///
        /// \param Bit The value asked about.
        /// \return `true` if the skin gives it, `false` if the element keeps its own.
        ZY_INLINE Bool Has(Field Bit) const
        {
            return HasAnyBit(Mask, Bit);
        }

        /// \brief Gives every value another skin gives, over the ones this one gives.
        ///
        /// \param Other The skin worn over this one, whose values win.
        void Merge(ConstRef<Skin> Other);

        /// \brief Gives a fill colour.
        ///
        /// \param Value The colour the box is filled with.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetBackground(Color Value);

        /// \brief Gives a border.
        ///
        /// \param Value The colour of the border.
        /// \param Width The thickness of the border, in pixels.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetBorder(Color Value, Real32 Width);

        /// \brief Gives a corner radius.
        ///
        /// \param Value The radius the corners are rounded to, in pixels.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetRadius(Real32 Value);

        /// \brief Gives an opacity.
        ///
        /// \param Value The opacity of the element and all it holds, over nought through one.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetOpacity(Real32 Value);

        /// \brief Gives a text colour.
        ///
        /// \param Value The colour the text is drawn in.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetTint(Color Value);

        /// \brief Gives a text size.
        ///
        /// \param Value The size the text is set at, in pixels.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetSize(Real32 Value);

        /// \brief Gives a typeface.
        ///
        /// \param Value The typeface the text is set in.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetFont(Retainer<ZyRender::Font> Value);

        /// \brief Gives an outline around the text.
        ///
        /// \param Value The colour of the outline.
        /// \param Width The thickness of the outline, in pixels.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetStroke(Color Value, Real32 Width);

        /// \brief Gives the room kept inside the box.
        ///
        /// \param Value The room on each side, in pixels.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetPadding(Edges Value);

        /// \brief Gives a width.
        ///
        /// \param Value The width the element is laid out at.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetWidth(Length Value);

        /// \brief Gives a height.
        ///
        /// \param Value The height the element is laid out at.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetHeight(Length Value);

        /// \brief Gives the colour of a bar's filled share.
        ///
        /// \param Value The colour the filled share is drawn in.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetFill(Color Value);

        /// \brief Gives the colour of a scroll's thumb.
        ///
        /// \param Value The colour the thumb is drawn in.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetThumb(Color Value);

        /// \brief Gives where each line of the text sits.
        ///
        /// \param Value The alignment of each line along the width of the box.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetAlignment(Align Value);

        /// \brief Gives the room between the children.
        ///
        /// \param Value The room left between one child and the next, in pixels.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetGap(Real32 Value);

        /// \brief Gives a gradient to fill the box with, over its background.
        ///
        /// \param From  The colour it starts from.
        /// \param To    The colour it ends at.
        /// \param Angle The direction it runs toward, in degrees, nought being upward and ninety to the right.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetGradient(Color From, Color To, Real32 Angle = 180.0f);

        /// \brief Gives a shadow cast behind the box.
        ///
        /// \param Value The shadow, with its colour, blur, spread and offset.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetShadow(ConstRef<Halo> Value);

        /// \brief Gives a glow around the box.
        ///
        /// \param Value The glow, whose offset is not read.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetGlow(ConstRef<Halo> Value);

        /// \brief Gives a glow just inside the box's edge.
        ///
        /// \param Value The glow, whose offset and spread are not read.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetInnerGlow(ConstRef<Halo> Value);

        /// \brief Gives how much larger the element is drawn, about its middle.
        ///
        /// \param Value The scale on each axis, one being its own size.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetScale(Vector2 Value);

        /// \brief Gives how far the element is drawn turned, about its middle.
        ///
        /// \param Value The turn, clockwise, in degrees.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetRotate(Real32 Value);

        /// \brief Gives how far the element is drawn moved.
        ///
        /// \param Value The move, in pixels.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetTranslate(Vector2 Value);

        /// \brief Gives how the element moves to the values a new state gives.
        ///
        /// \param Duration The time it takes, in seconds.
        /// \param Curve    The easing along the way.
        /// \param Delay    The time it waits before it starts, in seconds.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetTransition(Real32 Duration, Easing Curve = Easing::OutQuad, Real32 Delay = 0.0f);

        /// \brief Gives the animation the element plays, looping, while it wears the skin.
        ///
        /// \param Name The name of the animation, or empty for none.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetAnimation(Text Name);

        /// \brief Gives the animation the element plays once when it appears.
        ///
        /// \param Name The name of the animation, or empty for none.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetEnter(Text Name);

        /// \brief Gives the animation the element plays once when it is hidden, before it goes.
        ///
        /// \param Name The name of the animation, or empty for none.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetExit(Text Name);

        /// \brief Gives an image to lay over the box, between its fill and its picture.
        ///
        /// \param Value The image, or none to lay no image.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetImage(Retainer<ZyGraphic::Image> Value);

        /// \brief Gives the part of the image shown.
        ///
        /// \param Value The part, in texture coordinates.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetRegion(Rect Value);

        /// \brief Gives the border of the image kept unstretched, which slices it in nine.
        ///
        /// \param Value The border on each side, in pixels of the image, or none to stretch it whole.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetSlice(Edges Value);

        /// \brief Gives the colour the image is multiplied by.
        ///
        /// \param Value The colour every texel of the image is multiplied by.
        /// \return This skin, so further values chain onto it.
        Ref<Skin> SetImageTint(Color Value);

    private:

        /// \brief Takes a value another skin gives in place of this one's own.
        ///
        /// \param Gives The flag, `true` if the other skin gives the value, `false` to keep this one's.
        /// \param Value The value this skin holds, replaced when the other gives it.
        /// \param Given The value the other skin holds.
        template<typename Type>
        static void Adopt(Bool Gives, Ref<Type> Value, ConstRef<Type> Given);
    };
}