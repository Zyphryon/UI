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

#include <Zyphryon.Math/Color.hpp>
#include <Zyphryon.Math/Geometry/Rect.hpp>
#include <Zyphryon.Math/Vector2.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Specifies how the target the interface draws into stores colour.
    enum class Colorspace : UInt8
    {
        Linear,          ///< Stored as written, so a colour reaches the display unchanged.
        sRGB,            ///< Encoded on write, so a colour is decoded first to survive the round trip.
        sRGBUnmanaged,   ///< Stores sRGB but encodes none of it, so the encoding is done before handing it over.
    };

    /// \brief Represents the band a pane is drawn in, with where it stands among the others and what it does.
    struct Layer final
    {
        /// \brief Specifies what a band does beyond where it stands.
        enum class Trait : UInt8
        {
            None    = 0,        ///< It does nothing beyond where it stands.
            Cycled  = 1u << 0,  ///< Its panes take part when the keyboard moves the focus from pane to pane.
            Popup   = 1u << 1,  ///< Its panes close on a press outside them or on escape.
            Modal   = 1u << 2,  ///< Its panes hold the pointer and the keyboard, over a backdrop, until they close.
            Overlay = 1u << 3,  ///< Its panes are never hit and never closed by escape, as tooltips and ghosts.
        };
        ZY_DEFINE_BITWISE_FRIEND_ENUM(Trait)

        /// Where the band stands, the higher drawn over the lower.
        UInt8 Order  = 0;

        /// What the band does.
        Trait Traits = Trait::None;

        /// \brief Checks whether the band does something.
        ///
        /// \param Bit The trait to check for.
        /// \return `true` if it does, `false` otherwise.
        ZY_INLINE constexpr Bool Has(Trait Bit) const
        {
            return HasAnyBit(Traits, Bit);
        }

        /// \brief Checks whether two bands are the same.
        ///
        /// \param Other The band to compare against.
        /// \return `true` if they stand in the same place and do the same, `false` otherwise.
        constexpr Bool operator==(ConstRef<Layer> Other) const = default;
    };

    /// \brief Specifies the states the interface keeps for itself; a module declares more through \ref States.
    enum class State : UInt32
    {
        None     = 0,        ///< In none of the states.
        Hidden   = 1u << 0,  ///< Neither laid out, drawn nor hit, together with all it holds.
        Disabled = 1u << 1,  ///< Answering nothing, and drawn in its disabled look.
        Hover    = 1u << 2,  ///< The pointer rests on it or on something it holds.
        Press    = 1u << 3,  ///< Held down by the pointer.
        Focus    = 1u << 4,  ///< Holding the keyboard.
    };
    ZY_DEFINE_BITWISE_ENUM(State)

    /// \brief Specifies what of an element must be worked out again before it is next drawn.
    enum class Dirty : UInt8
    {
        None     = 0,                        ///< Nothing to work out again.
        Style    = 1u << 0,                  ///< Its look, after its state or flags changed.
        Measure  = 1u << 1,                  ///< Its size, and so the layout of what holds it.
        Paint    = 1u << 2,                  ///< What it draws, its size unchanged.
        Resize   = 1u << 3,                  ///< Its size again, as a child's size or the room for its text changed.
        Arrange  = 1u << 4,                  ///< Where its children go, as it or one of them was measured again.
        Below    = 1u << 5,                  ///< Nothing of its own, but a row it holds is to be arranged again.
        Turned   = 1u << 6,                  ///< No change at all, only its reach moved by its own transform.
        Unseen   = 1u << 7,                  ///< No change, only the last paint finding it out of view.
        All      = Style | Measure | Paint,  ///< Its look, its size and what it draws.
    };
    ZY_DEFINE_BITWISE_ENUM(Dirty)

    /// \brief Specifies the axis a node lays its children out along.
    enum class Direction : UInt8
    {
        Row,             ///< Children are laid left to right.
        Column,          ///< Children are laid top to bottom.
    };

    /// \brief Specifies the way the keyboard or a pad moves the focus across the screen.
    enum class Heading : UInt8
    {
        Up,              ///< Toward the top of the screen.
        Down,            ///< Toward the bottom of the screen.
        Left,            ///< Toward the left edge of the screen.
        Right,           ///< Toward the right edge of the screen.
    };

    /// \brief Specifies where a child sits across its parent's axis, or where text sits within its box.
    enum class Align : UInt8
    {
        Start,           ///< Against the leading edge.
        Center,          ///< In the middle.
        End,             ///< Against the trailing edge.
        Stretch,         ///< Across the whole of the room, which text reads as the leading edge.
    };

    /// \brief Specifies what a line of text too long for its box does.
    enum class Overflow : UInt8
    {
        Clip,            ///< Runs on past the box, cut wherever what holds it cuts.
        Ellipsis,        ///< Is cut short where it would overrun the box, an ellipsis standing for the rest.
    };

    /// \brief Specifies how the room left along a parent's axis is shared among its children.
    enum class Justify : UInt8
    {
        Start,           ///< Children gather at the leading edge.
        Center,          ///< Children gather in the middle.
        End,             ///< Children gather at the trailing edge.
        Between,         ///< The room is spread evenly between the children.
    };

    /// \brief Specifies whether a node takes a place in its parent's flow, or stands over it.
    enum class Position : UInt8
    {
        Flow,            ///< Laid out among its siblings.
        Absolute,        ///< Placed by its distance from the parent's edges, over whatever flows there.
    };

    /// \brief Represents a size along one axis, in pixels, as a share of the parent, or taken from the content.
    struct Length final
    {
        /// \brief Specifies what the value of a length is measured in.
        enum class Unit : UInt8
        {
            Auto,        ///< The size the content asks for, the value being ignored.
            Pixel,       ///< The value, in pixels.
            Percent,     ///< The value as a share of the parent's room, over nought through one.
            Fit,         ///< The size the content asks for, up to the value in pixels, its content scrolling past it.
        };

        /// The measure of the length, read the way its unit says.
        Real32 Value   = 0.0f;

        /// What the value is measured in.
        Unit   Measure = Unit::Auto;

        /// \brief Constructs a length taken from the content.
        ZY_INLINE constexpr Length() = default;

        /// \brief Constructs a length of the given value and unit.
        ///
        /// \param Value   The measure of the length.
        /// \param Measure The unit the measure is read in.
        ZY_INLINE constexpr Length(Real32 Value, Unit Measure)
            : Value   { Value },
              Measure { Measure }
        {
        }

        /// \brief Checks whether the length is taken from the content.
        ///
        /// \return `true` if the content decides it, `false` otherwise.
        ZY_INLINE constexpr Bool IsAuto() const
        {
            return Measure == Unit::Auto;
        }

        /// \brief Resolves the length against the room it is laid out in.
        ///
        /// \param Room    The room of the parent, in pixels.
        /// \param Content The size the content asks for, in pixels.
        /// \return The size, in pixels.
        ZY_INLINE constexpr Real32 Resolve(Real32 Room, Real32 Content) const
        {
            switch (Measure)
            {
            case Unit::Pixel:
                return Value;
            case Unit::Percent:
                return Value * Room;
            case Unit::Fit:
                return Min(Content, Value);
            default:
                return Content;
            }
        }

        /// \brief Gets the size the length asks for before the parent's room is known.
        ///
        /// \param Content The size the content asks for, in pixels.
        /// \return The size, in pixels, the content's for a share of the room, which is only known once placed.
        ZY_INLINE constexpr Real32 Ask(Real32 Content) const
        {
            switch (Measure)
            {
            case Unit::Pixel:
                return Value;
            case Unit::Fit:
                return Min(Content, Value);
            default:
                return Content;
            }
        }

        /// \brief Creates a length taken from the content.
        ///
        /// \return The length, which resolves to the size the content asks for.
        ZY_INLINE static constexpr Length Auto()
        {
            return Length();
        }

        /// \brief Creates a length taken from the content, up to a size it never grows past.
        ///
        /// \param Most The most it grows to, in pixels.
        /// \return The length, which resolves to the content's size held under \p Most.
        ZY_INLINE static constexpr Length Fit(Real32 Most)
        {
            return Length(Most, Unit::Fit);
        }

        /// \brief Creates a length of so many pixels.
        ///
        /// \param Value The count of pixels.
        /// \return The length, which resolves to the count of pixels whatever the room.
        ZY_INLINE static constexpr Length Pixels(Real32 Value)
        {
            return Length(Value, Unit::Pixel);
        }

        /// \brief Creates a length as a share of the parent's room.
        ///
        /// \param Value The share, over nought through one.
        /// \return The length, which resolves to the share of the room it is laid out in.
        ZY_INLINE static constexpr Length Percent(Real32 Value)
        {
            return Length(Value, Unit::Percent);
        }

        /// \brief Checks whether two lengths are the same.
        ///
        /// \param Other The length to compare against.
        /// \return `true` if both value and unit match, `false` otherwise.
        ZY_INLINE constexpr Bool operator==(ConstRef<Length> Other) const = default;
    };

    /// \brief Represents a thickness on each of the four sides of a box, in pixels.
    struct Edges final
    {
        /// The thickness on the left side.
        Real32 Left   = 0.0f;

        /// The thickness on the top side.
        Real32 Top    = 0.0f;

        /// The thickness on the right side.
        Real32 Right  = 0.0f;

        /// The thickness on the bottom side.
        Real32 Bottom = 0.0f;

        /// \brief Constructs edges of no thickness at all.
        ZY_INLINE constexpr Edges() = default;

        /// \brief Constructs edges of the same thickness on every side.
        ///
        /// \param All The thickness of every side.
        ZY_INLINE constexpr explicit Edges(Real32 All)
            : Edges(All, All, All, All)
        {
        }

        /// \brief Constructs edges of the given thickness on each side.
        ///
        /// \param Left   The thickness on the left side.
        /// \param Top    The thickness on the top side.
        /// \param Right  The thickness on the right side.
        /// \param Bottom The thickness on the bottom side.
        ZY_INLINE constexpr Edges(Real32 Left, Real32 Top, Real32 Right, Real32 Bottom)
            : Left   { Left },
              Top    { Top },
              Right  { Right },
              Bottom { Bottom }
        {
        }

        /// \brief Checks whether every side has no thickness.
        ///
        /// \return `true` if all four sides are nought, `false` otherwise.
        ZY_INLINE constexpr Bool IsEmpty() const
        {
            return Left <= 0.0f && Top <= 0.0f && Right <= 0.0f && Bottom <= 0.0f;
        }

        /// \brief Shrinks a box by the edges, its right and bottom sides never crossing its left and top ones.
        ///
        /// \param Box The box to shrink.
        /// \return The box left inside the edges.
        ZY_INLINE constexpr Rect Inset(Rect Box) const
        {
            const Real32 MinimumX = Box.GetMinimumX() + Left;
            const Real32 MinimumY = Box.GetMinimumY() + Top;

            return Rect(
                MinimumX,
                MinimumY,
                Max(Box.GetMaximumX() - Right,  MinimumX),
                Max(Box.GetMaximumY() - Bottom, MinimumY));
        }

        /// \brief Checks whether two sets of edges are the same.
        ///
        /// \param Other The edges to compare against.
        /// \return `true` if every side matches, `false` otherwise.
        ZY_INLINE constexpr Bool operator==(ConstRef<Edges> Other) const = default;
    };

    /// \brief Represents a soft rounded box drawn with an element, as a shadow behind it or a glow around or within it.
    struct Halo final
    {
        /// The colour at its most solid, where a transparent one draws nothing.
        Color   Tint   = Color::Transparent();

        /// How far it is moved from the element, in pixels, as a light above casts a shadow downward.
        Vector2 Offset;

        /// How far its edge fades out on either side, in pixels.
        Real32  Blur   = 0.0f;

        /// How far it grows past the element's box before it fades, in pixels.
        Real32  Spread = 0.0f;

        /// \brief Checks whether it draws anything.
        ///
        /// \return `true` if its colour is not wholly transparent, `false` otherwise.
        ZY_INLINE Bool IsVisible() const
        {
            return Tint.GetAlpha() > 0.0f;
        }
    };

    /// \brief Represents a fill fading from one colour to another along a direction, in place of a flat one.
    struct Gradient final
    {
        /// The colour it starts from.
        Color  From  = Color::Transparent();

        /// The colour it ends at.
        Color  To    = Color::Transparent();

        /// The direction it runs toward, in degrees as CSS reads them.
        Real32 Angle = 180.0f;

        /// \brief Checks whether it draws anything.
        ///
        /// \return `true` if either colour is not wholly transparent, `false` otherwise.
        ZY_INLINE Bool IsVisible() const
        {
            return From.GetAlpha() > 0.0f || To.GetAlpha() > 0.0f;
        }
    };
}