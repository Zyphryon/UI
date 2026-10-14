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
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents a scroll viewer, a view over larger content with a bar down its side and along its bottom.
    class Scroller final
    {
    public:

        /// \brief Creates a scroll viewer, scrolling both ways.
        ///
        /// \param Parent The element to hold it.
        /// \param Style  The name of the style it wears.
        /// \return The viewer, two hundred pixels tall until it is given a height.
        static Element Create(Element Parent, Text Style = "Scroller");

        /// \brief Creates a scroll viewer from a node, reading `Horizontal` and `Follow`, remembered by `Name`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the viewer is read from.
        /// \return The viewer, whose content takes the node's children.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Gets the part of a scroll viewer its children stand in, which scrolls.
        ///
        /// \param Viewer The viewer whose content is wanted.
        /// \return The panel that scrolls within the viewer.
        static Element GetContent(Element Viewer);

        /// \brief Gets one of the scroll bars of a scroll viewer.
        ///
        /// \param Viewer   The viewer whose bar is wanted.
        /// \param Vertical The flag, `true` to get the bar down its side, `false` to get the one along its bottom.
        /// \return The bar, hidden while there is nothing to scroll its way.
        static Element GetBar(Element Viewer, Bool Vertical);

        /// \brief Lets a scroll viewer's content keep its width and scroll sideways, or fits it to the viewer's width.
        ///
        /// \param Viewer     The viewer to change.
        /// \param Horizontal The flag, `true` to scroll sideways, `false` to fit the content to the width.
        static void SetHorizontal(Element Viewer, Bool Horizontal);

        /// \brief Keeps a scroll viewer at its end however much it holds, as a log, until it is scrolled away from it.
        ///
        /// \param Viewer The viewer to change.
        /// \param Follow The flag, `true` to keep it at its end, `false` to leave it where it is put.
        static void SetFollow(Element Viewer, Bool Follow);

        /// \brief Scrolls a scroll viewer, held within what it holds, without reporting it.
        ///
        /// \param Viewer The viewer to scroll.
        /// \param Offset The distance right and down from the start, in pixels.
        static void ScrollTo(Element Viewer, Vector2 Offset);

        /// \brief Gets how far a scroll viewer is scrolled.
        ///
        /// \param Viewer The viewer to read.
        /// \return The distance right and down from the start, in pixels.
        static Vector2 GetOffset(Element Viewer);

        /// \brief Makes the interface remember a viewer's scroll under a key, restoring it once content reaches it.
        ///
        /// \param Viewer The viewer remembered.
        /// \param Key    The key, unique among what the interface remembers, or empty to remember it no more.
        static void Remember(Element Viewer, Text Key);

    private:

        /// How tall a viewer stands until given a height, in pixels, since one sized by its content never scrolls.
        static constexpr Real32 kHeight = 200.0f;

        /// How far an arrow key scrolls a viewer holding the keyboard, in pixels, as a notch of the wheel does.
        static constexpr Real32 kStep   = 40.0f;

        /// \brief Runs a viewer holding the keyboard itself, where the arrows, the page keys, home and end scroll it.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it.
        /// \param Self    The viewer hearing the event.
        /// \return `true` if a key scrolled it, `false` otherwise, so at an end the arrow moves the keyboard on.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Scrolls a viewer by a key.
        ///
        /// \param Viewer The viewer to scroll.
        /// \param Key    The key pressed while it holds the keyboard.
        /// \return `true` if it moved, `false` otherwise.
        static Bool OnKey(Element Viewer, ZyInput::Key Key);

        /// \brief Keeps how far a remembered viewer is scrolled, or brings it back to where it was left.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The viewer remembered.
        /// \return `true` if it was scrolled back, `false` otherwise.
        static Bool OnRealize(Ref<Interface> Owner, Handle Self);

        /// \brief Points one of a viewer's bars at its content, shown only while something scrolls, hidden at first.
        ///
        /// \param Bar     The bar to point.
        /// \param Content The content of the viewer, which the bar scrolls.
        static void Hook(Element Bar, Element Content);
    };
}