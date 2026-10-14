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
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the state of a scroll bar, with the element it scrolls, its thumb, and where it was grabbed.
    struct Rail final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The element it scrolls, which holds a scroll, or one naming nothing.
        Handle Target;

        /// The identifier of the element it scrolls, looked up in its pane while `Target` names nothing.
        UInt64 Name      = 0;

        /// The thumb, which the bar sizes and places after every layout.
        Handle Thumb;

        /// `true` when it runs up and down, `false` when it runs sideways.
        Bool   Vertical  = true;

        /// `true` when it hides while what it scrolls fits.
        Bool   Automatic = false;

        /// How far along the thumb the pointer took it, in pixels.
        Real32 Grab      = 0.0f;
    };

    /// \brief Represents a scroll bar, a track and a thumb sized to the view, scrolling an element with a \ref Scroll.
    class Scrollbar final
    {
    public:

        /// How thick a bar is when its style gives no size, in pixels.
        static constexpr Real32 kThickness = 12.0f;

        /// How short a thumb gets over a very long content, in pixels, so it can still be taken.
        static constexpr Real32 kShortest  = 16.0f;

        /// How far one notch of the wheel over a bar scrolls, in pixels.
        static constexpr Real32 kNotch     = 40.0f;

    public:

        /// \brief Creates a scroll bar.
        ///
        /// \param Parent   The element to hold it.
        /// \param Vertical The flag, `true` to run it up and down, `false` to run it sideways.
        /// \param Style    The name of the style it wears, its thumb wearing it followed by `.Thumb`.
        /// \return The bar, twelve pixels thick unless its style gives its width, or its height when it runs sideways.
        static Element Create(Element Parent, Bool Vertical = true, Text Style = "Scrollbar");

        /// \brief Creates a scroll bar from a layout node, reading `Orientation`, `Target` by name and `Automatic`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the bar is read from.
        /// \return The bar, which finds its target by name once the layout stands.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Points a scroll bar at the element it scrolls, making that element scroll the way the bar runs.
        ///
        /// \param Bar    The bar to point.
        /// \param Target The element the bar scrolls from now on.
        static void SetTarget(Element Bar, Element Target);

        /// \brief Gets the element a scroll bar scrolls, looking it up by name the first time it is needed.
        ///
        /// \param Bar The bar to read.
        /// \return The element, or one naming nothing.
        static Element GetTarget(Element Bar);

        /// \brief Sets whether a scroll bar hides while what it scrolls fits, or always shows.
        ///
        /// \param Bar       The bar to change.
        /// \param Automatic The flag, `true` to hide it while nothing is left to scroll, `false` to always show it.
        static void SetAutomatic(Element Bar, Bool Automatic);

        /// \brief Gets the thumb of a scroll bar.
        ///
        /// \param Bar The bar to read.
        /// \return The thumb, which wears the bar's style followed by `.Thumb`.
        static Element GetThumb(Element Bar);

        /// \brief Scrolls an element along one axis, held within what it holds, without reporting it.
        ///
        /// \param Target   The element, which holds a scroll.
        /// \param Vertical The flag, `true` to scroll it up and down, `false` to scroll it sideways.
        /// \param Offset   The distance from the start, in pixels.
        /// \return `true` if it moved, `false` otherwise.
        static Bool ScrollTo(Element Target, Bool Vertical, Real32 Offset);

        /// \brief Gets how far an element is scrolled along one axis.
        ///
        /// \param Target   The element, which holds a scroll.
        /// \param Vertical The flag, `true` to read it up and down, `false` to read it sideways.
        /// \return How far from the start, in pixels, or nought when it does not scroll.
        static Real32 GetOffset(Element Target, Bool Vertical);

        /// \brief Gets how far an element can scroll along one axis, as the last layout found it.
        ///
        /// \param Target   The element, which holds a scroll.
        /// \param Vertical The flag, `true` to read it up and down, `false` to read it sideways.
        /// \return The farthest offset, in pixels, or nought when everything fits.
        static Real32 GetLimit(Element Target, Bool Vertical);

        /// \brief Gets how much of an element is seen along one axis, as the last layout found it.
        ///
        /// \param Target   The element, which holds a scroll.
        /// \param Vertical The flag, `true` to get its height, `false` to get its width.
        /// \return The room it is seen through, in pixels.
        static Real32 GetView(Element Target, Bool Vertical);

    private:

        /// \brief Runs a bar, where a press on its track pages toward the pointer and the wheel scrolls along it.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching the bar.
        /// \param Self    The bar the handler governs.
        /// \return `true` if it scrolled or took the press or the turn, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Runs a thumb, where a press takes it where it was pressed and dragging it scrolls in step.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching the thumb.
        /// \param Self    The thumb the handler governs.
        /// \return `true` if the event was a press or a drag on it, `false` otherwise.
        static Bool OnThumb(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Fits a bar to what it scrolls as the last layout left it, showing or hiding it and placing its thumb.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The bar to fit.
        /// \return `true` if the bar or its thumb changed and the pane must be laid out again, `false` otherwise.
        static Bool OnRealize(Ref<Interface> Owner, Handle Self);

        /// \brief Makes the element a bar scrolls able to scroll the way the bar runs.
        ///
        /// \param Bar    The bar whose way is read.
        /// \param Target The element the bar scrolls.
        static void Prepare(Element Bar, Element Target);

        /// \brief Places a bar's thumb along its track.
        ///
        /// \param Bar   The bar whose thumb is placed.
        /// \param Start The distance along the track where the thumb starts, in pixels.
        /// \param Size  The length of the thumb, in pixels.
        /// \return `true` if the thumb moved or changed length, `false` otherwise.
        static Bool Fit(Element Bar, Real32 Start, Real32 Size);

        /// \brief Gets the track of a bar, inside its padding, where the thumb runs.
        ///
        /// \param Bar The bar, which was laid out.
        /// \return The track, in pixels of the target.
        static Rect GetTrack(Element Bar);

        /// \brief Checks whether a length stands at so many pixels, near enough that moving it changes nothing seen.
        ///
        /// \param Value  The length to check.
        /// \param Pixels The count of pixels it should stand at.
        /// \return `true` if it does, `false` otherwise.
        static Bool Near(ConstRef<Length> Value, Real32 Pixels);
    };
}