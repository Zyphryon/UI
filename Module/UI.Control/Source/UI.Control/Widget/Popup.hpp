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
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents a popup, a pane over everything else, opened by a control and closed by a press outside both.
    class Popup final
    {
    public:

        /// \brief Creates a popup, closed.
        ///
        /// \param Opener The control that opens it, which hears its events.
        /// \param Style  The name of the style it wears.
        /// \return The root of the popup's pane, which holds what the popup shows.
        static Element Create(Element Opener, Text Style);

        /// \brief Opens a popup beside an element, over everything else, even a modal pane.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The root of the popup to open.
        /// \param Anchor The element it opens beside.
        /// \param Below  The flag, `true` to open under the anchor and as wide as it, `false` to open to its right.
        static void Show(Ref<Interface> Owner, Handle Target, Handle Anchor, Bool Below = true);

        /// \brief Opens a popup under an element, left aligned, as wide as its content, over all, even a modal pane.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The root of the popup to open, as a menu dropped from a menu bar's title.
        /// \param Anchor The element it opens under.
        static void Drop(Ref<Interface> Owner, Handle Target, Handle Anchor);

        /// \brief Closes a popup.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The root of the popup to close.
        static void Hide(Ref<Interface> Owner, Handle Target);

        /// \brief Checks whether a popup is open.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The root of the popup to look at.
        /// \return `true` if it is, `false` otherwise.
        static Bool IsOpen(ConstRef<Interface> Owner, Handle Target);

    private:

        /// \brief Shows a popup beside its anchor, in the band over the anchor's, above every other pane there.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The root of the popup to show.
        /// \param Anchor The element it was placed beside, whose band it may take.
        static void Lift(Ref<Interface> Owner, Handle Target, Handle Anchor);
    };
}