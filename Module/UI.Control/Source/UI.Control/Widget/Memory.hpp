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
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents an element its interface remembers by a key, so one made again takes back how it was left.
    struct Recall final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The identifier of the key the element is remembered by.
        UInt64  Key       = 0;

        /// The scroll, right and down in pixels, the element is brought back to once what it holds reaches that far.
        Vector2 Wanted;

        /// The scroll seen at the last update while it waited, so a move the user makes ends the wait.
        Vector2 Seen;

        /// `true` while the element waits for what it holds to reach the scroll it is brought back to.
        Bool    Restoring = false;
    };

    /// \brief Provides what widgets remember of themselves, as an expander's open state and a list's scroll.
    class Memory final
    {
    public:

        /// \brief Gets the key a layout widget is remembered by, its `Name` within the scope it was made in.
        ///
        /// \param Reader The markup reading the layout, whose scope tells the records apart.
        /// \param Node   The layout node, whose `Name` names the widget.
        /// \return The key, empty when the node names nothing, so the widget remembers nothing.
        static Str GetKey(ConstRef<Markup> Reader, ConstRef<JsonValue> Node);

        /// \brief Remembers an element under a key, taking back its scroll if it holds a \ref Scroll.
        ///
        /// \param Target The element remembered.
        /// \param Key    The key, or empty to remember it no more.
        static void Hold(Element Target, Text Key);

        /// \brief Keeps whether an element remembered by a key is open, as an expander does when it turns.
        ///
        /// \param Target The element, which keeps nothing when it is remembered by no key.
        /// \param Opened The flag, `true` when it is open, `false` when it is closed.
        static void SetOpened(Element Target, Bool Opened);

        /// \brief Finds whether an element remembered by a key was left open.
        ///
        /// \param Target The element, remembered by the key it was held under.
        /// \param Output The flag receiving `true` when it was left open, `false` when it was left closed.
        /// \return `true` if the key remembers an open state, `false` when the element is new to it.
        static Bool FindOpened(Element Target, Ref<Bool> Output);

        /// \brief Keeps how far a remembered element is scrolled, restoring it once what it holds reaches that far.
        ///
        /// \param Target The scrolling element, which keeps nothing when it is remembered by no key.
        /// \return `true` if it scrolled the element, so the tree is laid out again, `false` otherwise.
        static Bool Track(Element Target);

    private:

        /// \brief Holds what an interface remembers, by the identifier of each key.
        struct Store final
        {
            /// Whether each element remembered was left open.
            Table<Digest, Bool>    Opened;

            /// How far each element remembered was left scrolled, right and down in pixels.
            Table<Digest, Vector2> Scrolled;
        };

        /// \brief Gets what an interface remembers, keeping it on the tree the first time.
        ///
        /// \param Owner The tree the memory is kept on.
        /// \return The memory of the tree.
        static Ref<Store> Supply(Ref<Tree> Owner);
    };
}