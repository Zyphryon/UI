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
    /// \brief Represents what a menu bar keeps, the part at its far end and the title whose menu stands open.
    struct Strip final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The name of the style the bar wears, its titles wearing it followed by `.Title`.
        Str    Style;

        /// The part at the far end of the bar, holding what stands past the menus, as a readout or a button.
        Handle Trail;

        /// The title whose menu is open, or one naming nothing while every menu is closed.
        Handle Open;

        /// The title the pointer rested on at the last update, so the keyboard keeps its menu until the pointer moves.
        Handle Hovered;
    };

    /// \brief Represents a menu bar, a row of titles each opening its menu under it, then what stands at its far end.
    class Menubar final
    {
    public:

        /// \brief Creates a menu bar holding no menu.
        ///
        /// \param Parent The element to hold it.
        /// \param Style  The name of the style it wears, its parts adding `.Title`, `.Title.Label` and `.Trail`.
        /// \return The menu bar.
        static Element Create(Element Parent, Text Style = "Menubar");

        /// \brief Creates a menu bar from a layout node, with its `Menus` and the `Children` at its far end.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the menu bar is read from.
        /// \return The menu bar, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Adds a menu to a menu bar, its title standing after the titles added before it.
        ///
        /// \param Bar     The menu bar the menu is added to.
        /// \param Content The text of its title.
        /// \param Style   The name of the style the menu wears.
        /// \return The menu, closed, which takes items through \ref Menu::AddItem and the like.
        static Element AddMenu(Element Bar, Text Content, Text Style = "Menu");

        /// \brief Gets the menu a title of a menu bar opens.
        ///
        /// \param Title The title, as \ref GetTitle gives it or as found by its name.
        /// \return The menu, or one naming nothing when the element is no title.
        static Element GetMenu(Element Title);

        /// \brief Gets the title opening a menu of a menu bar.
        ///
        /// \param Menu The menu, as \ref AddMenu gives it.
        /// \return The title, or one naming nothing when the menu belongs to no menu bar.
        static Element GetTitle(Element Menu);

        /// \brief Changes the text of a title of a menu bar.
        ///
        /// \param Title   The title whose text changes.
        /// \param Content The text shown on it.
        static void SetTitle(Element Title, Text Content);

        /// \brief Gets the part at the far end of a menu bar, which holds what stands past its menus.
        ///
        /// \param Bar The menu bar to look in.
        /// \return The part, where the children a layout gives the bar go.
        static Element GetTrail(Element Bar);

        /// \brief Opens the menu of a title under it, closing any other menu of its bar.
        ///
        /// \param Owner    The interface it stands in.
        /// \param Title    The title whose menu opens.
        /// \param Keyboard The flag, `true` to light the menu's first item, as the keyboard opens it, `false` for none.
        static void Open(Ref<Interface> Owner, Handle Title, Bool Keyboard = false);

        /// \brief Closes the menu of a menu bar that stands open, giving the keyboard back to what held it before.
        ///
        /// \param Owner The interface it stands in.
        /// \param Bar   The menu bar whose menu closes.
        static void Close(Ref<Interface> Owner, Handle Bar);

        /// \brief Gets the title of a menu bar whose menu stands open.
        ///
        /// \param Bar The menu bar to look at.
        /// \return The title, or one naming nothing while every menu is closed.
        static Element GetOpen(Element Bar);

    private:

        /// \brief Runs a menu bar, where a click on a title opens or closes its menu, and left and right move between.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching the bar.
        /// \param Self    The menu bar hearing the event.
        /// \return `true` if the event was the bar's, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Follows the menu open from a bar, freeing its title once closed and opening the pointer's title.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The menu bar followed.
        /// \return `true` if another menu opened, `false` otherwise.
        static Bool Track(Ref<Interface> Owner, Handle Self);

        /// \brief Opens the menu of the next usable title, left or right of the one open, wrapping past either end.
        ///
        /// \param Owner     The interface it stands in.
        /// \param Self      The menu bar whose titles are stepped through.
        /// \param Direction The direction, one to go right or minus one to go left.
        /// \return `true` if a menu stands open to move from, `false` otherwise.
        static Bool Shift(Ref<Interface> Owner, Handle Self, SInt32 Direction);

        /// \brief Checks whether a title of a menu bar can open its menu, as one that is neither hidden nor disabled.
        ///
        /// \param Owner The interface it stands in.
        /// \param Title The element to check.
        /// \return `true` if it can, `false` otherwise.
        static Bool IsUsable(ConstRef<Interface> Owner, Handle Title);

        /// \brief Adds one menu a layout writes to a menu bar, with its title's fields and its items.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Bar    The menu bar the menu is added to.
        /// \param Entry  The menu as the layout writes it, an object of `Text`, `Name`, `Hidden`, `Disabled`, `Items`.
        /// \param Style  The name of the style the menu wears.
        static void Add(Ref<Markup> Reader, Element Bar, ConstRef<JsonValue> Entry, Text Style);
    };
}