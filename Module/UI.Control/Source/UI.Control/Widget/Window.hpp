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
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the hold the pointer keeps on a window while it drags or sizes it, and the parts it holds.
    struct Grip final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The pointer's distance from the window's top left corner on a drag, or bottom right on a sizing, in pixels.
        Vector2 Offset;

        /// `true` while the pointer holds the window by its title bar.
        Bool    Held    = false;

        /// `true` while the pointer sizes the window by one of its edges.
        Bool    Sizing  = false;

        /// The axes the edge held sizes the window along, one for the width and one for the height.
        Vector2 Axes;

        /// The least size a sizing leaves the window, in pixels.
        Vector2 Minimum = Vector2(160.0f, 100.0f);

        /// The label on its title bar, kept since a window made inside a walk cannot find it before the walk ends.
        Handle  Title;

        /// The panel over the window holding the edges it is sized by, or one naming nothing until made resizable.
        Handle  Sizer;

        /// The style the window wears, which its edges wear followed by `.Edge` and `.Grip`.
        Str     Style;
    };

    /// \brief Represents a window, a pane dragged by its title bar, sized by its edges, closed by a button on it.
    class Window final
    {
    public:

        /// The band windows open in.
        static constexpr Layer  kLayer  = { 64, Layer::Trait::Cycled };

        /// The event reported before the close button closes a window; a handler stopping it leaves the window open.
        static constexpr UInt64 kClose  = Identify("Close");

        /// The thickness of the right and bottom edges a resizable window is sized by, in pixels.
        static constexpr Real32 kEdge   = 5.0f;

        /// The size of the grip at the bottom right corner of a resizable window, in pixels.
        static constexpr Real32 kCorner = 14.0f;

    public:

        /// \brief Opens a window in a pane of its own, shown.
        ///
        /// \param Target The tree to open the pane in.
        /// \param Title  The text on its title bar.
        /// \param Style  The name of the style it wears.
        /// \param Band   The band its pane is drawn in, as \ref Layers::kModal for a dialog.
        /// \return The window, reporting \ref Window::kClose on its close button, then \ref Signal::kChange on hiding.
        static Element Open(Ref<Tree> Target, Text Title, Text Style = "Window", Layer Band = kLayer);

        /// \brief Creates a window from a node with `Title` and `Resizable`, placed by `Left` and `Top`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element holding the node, whose tree the window opens in.
        /// \param Node   The layout node the window is read from.
        /// \return The window, opened as \ref Open opens one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Hides a window, without reporting it.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The window to hide.
        static void Close(Ref<Interface> Owner, Handle Target);

        /// \brief Asks to close a window as its close button does, hiding it unless a \ref kClose handler refuses.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The window to close.
        /// \return `true` if it closed, `false` if a handler refused.
        static Bool Request(Ref<Interface> Owner, Handle Target);

        /// \brief Shows a window, over every other window.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The window to show.
        static void Show(Ref<Interface> Owner, Handle Target);

        /// \brief Gets the panel a window holds its content in.
        ///
        /// \param Target The window whose content is wanted.
        /// \return The panel under the title bar, where the window's children go.
        static Element GetContent(Element Target);

        /// \brief Moves a window, its top left corner to a point of the target.
        ///
        /// \param Target   The window to move.
        /// \param Position The point, in pixels of the target.
        static void MoveTo(Element Target, Vector2 Position);

        /// \brief Changes the title on a window's bar.
        ///
        /// \param Target The window whose title changes.
        /// \param Title  The text to show on its title bar.
        static void SetTitle(Element Target, Text Title);

        /// \brief Lets the pointer size a window by its right and bottom edges and the grip at its corner, or stops it.
        ///
        /// \param Target    The window to make resizable.
        /// \param Resizable The flag, `true` to let the pointer size it, `false` otherwise.
        static void SetResizable(Element Target, Bool Resizable);

        /// \brief Checks whether the pointer can size a window.
        ///
        /// \param Target The window asked about.
        /// \return `true` if it can, `false` otherwise.
        static Bool IsResizable(Element Target);

        /// \brief Sets the least size a sizing leaves a window.
        ///
        /// \param Target  The window whose least size is set.
        /// \param Minimum The least width and height, in pixels.
        static void SetMinimum(Element Target, Vector2 Minimum);

        /// \brief Gets the grip at the bottom right corner of a resizable window.
        ///
        /// \param Target The window whose grip is wanted.
        /// \return The grip, or one naming nothing while the window is not resizable.
        static Element GetGrip(Element Target);

        /// \brief Runs a window, where its title bar moves it, its edges size it, and its close button closes it.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it.
        /// \param Self    The window hearing the event.
        /// \return `true` if the event moved, sized, held, let go or closed it, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Gets the close button of a window.
        ///
        /// \param Target The window whose close button is wanted.
        /// \return The button at the end of its title bar.
        static Element GetClose(Element Target);

    private:

        /// \brief Takes hold of a window when a press lands on its title bar, away from the close button, or an edge.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The press reaching it.
        /// \param Self    The window pressed.
        /// \return `true` if it took hold, `false` otherwise.
        static Bool OnPress(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Sizes a window held by an edge to the pointer, within its least size and the target's edge.
        ///
        /// \param Owner The interface it stands in.
        /// \param Root  The window sized.
        /// \param Point The pointer, in units of the interface.
        static void Size(Ref<Interface> Owner, Element Root, Vector2 Point);

        /// \brief Gets the title bar of a window.
        ///
        /// \param Target The window whose title bar is wanted.
        /// \return The bar holding its title and its close button.
        static Element GetBar(Element Target);
    };
}