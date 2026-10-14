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
    /// \brief Represents the tab a tab control shows, and the parts its tabs and pages stand in.
    struct Deck final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The row the tab buttons stand in.
        Handle Header;

        /// The panel the pages stand in, one for each tab and in the same order.
        Handle Pages;

        /// The index of the tab shown.
        UInt32 Selected = 0;
    };

    /// \brief Represents a tab control, a row of tab buttons over pages, showing only the picked tab's page.
    class Tabs final
    {
    public:

        /// The event reported on a tab's close button click, `Other` naming it; a handler stopping it keeps the tab.
        static constexpr UInt64 kClose = Identify("TabClose");

    public:

        /// \brief Creates a tab control holding no tab.
        ///
        /// \param Parent The element to hold it.
        /// \param Style  The name of the style it wears, with parts `.Header`, `.Tab`, `.Close`, `.Pages`, `.Page`.
        /// \return The tab control, reporting \ref Signal::kChange on a pick and \ref Tabs::kClose on a close click.
        static Element Create(Element Parent, Text Style = "Tabs");

        /// \brief Creates a tab control from a layout node, holding its `Tabs` and showing `Selected`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the tab control is read from.
        /// \return The tab control, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Adds a tab, its page shown only when picked, dressed in the given style.
        ///
        /// \param Owner    The tab control to add the tab to.
        /// \param Title    The text on the tab.
        /// \param Style    The name of the style the tab control wears.
        /// \param Closable The flag, `true` to give the tab a close button after its title, `false` otherwise.
        /// \return The page, to hold what the tab shows.
        static Element AddTab(Element Owner, Text Title, Text Style = "Tabs", Bool Closable = false);

        /// \brief Gives a tab a close button after its title, or takes it away.
        ///
        /// \param Owner    The tab control holding the tab.
        /// \param Index    The index of the tab, left alone when it names none.
        /// \param Closable The flag, `true` to give it a close button, `false` to take the button away.
        /// \param Style    The name of the style the tab control wears, which the button wears followed by `.Close`.
        static void SetClosable(Element Owner, UInt32 Index, Bool Closable, Text Style = "Tabs");

        /// \brief Gets the close button of a tab.
        ///
        /// \param Owner The tab control holding the tab.
        /// \param Index The index of the tab.
        /// \return The button after the tab's title, or one naming nothing for a tab with none or past the last.
        static Element GetClose(Element Owner, UInt32 Index);

        /// \brief Takes a tab, its page and its contents away, showing the next tab; move out what must live on.
        ///
        /// \param Owner The tab control to take the tab from.
        /// \param Index The index of the tab, left alone when it names none.
        static void RemoveTab(Element Owner, UInt32 Index);

        /// \brief Shows the page of a tab of a tab control, without reporting it.
        ///
        /// \param Owner The tab control to pick in.
        /// \param Index The index of the tab, left alone when it names none.
        static void Select(Element Owner, UInt32 Index);

        /// \brief Gets the index of the tab a tab control shows.
        ///
        /// \param Owner The tab control to read.
        /// \return The index of the tab shown, counted from the first.
        static UInt32 GetSelected(Element Owner);

        /// \brief Gets how many tabs a tab control holds.
        ///
        /// \param Owner The tab control to count.
        /// \return The number of tabs.
        static UInt32 GetCount(Element Owner);

        /// \brief Gets a tab button of a tab control.
        ///
        /// \param Owner The tab control to look in.
        /// \param Index The index of the tab.
        /// \return The tab button, or one naming nothing past the last tab.
        static Element GetTab(Element Owner, UInt32 Index);

        /// \brief Gets the page of a tab of a tab control.
        ///
        /// \param Owner The tab control to look in.
        /// \param Index The index of the tab.
        /// \return The page, or one naming nothing past the last tab.
        static Element GetPage(Element Owner, UInt32 Index);

    private:

        /// \brief Adds a tab out of an entry of a node's `Tabs`, building the entry's `Children` into its page.
        ///
        /// \param Reader   The markup reading the layout.
        /// \param Owner    The tab control to add the tab to.
        /// \param Node     The entry, giving `Title`, `Name`, `Closable` and `Children`.
        /// \param Style    The name of the style the tab control wears.
        /// \param Closable The flag, `true` when the control gives tabs a close button unless the entry says not.
        static void BuildTab(Ref<Markup> Reader, Element Owner, ConstRef<JsonValue> Node, Text Style, Bool Closable);

        /// \brief Reports a tab's close button clicked, and takes the tab away unless a handler stops the report.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The tab control holding the tab.
        /// \param Tab   The tab whose close button was clicked.
        static void Shut(Ref<Interface> Owner, Handle Self, Element Tab);

        /// \brief Gives a tab button a close button after its title, or takes it away.
        ///
        /// \param Tab      The tab button, which may still wait for the walk it was made in to end.
        /// \param Closable The flag, `true` to give it a close button, `false` to take the button away.
        /// \param Style    The name of the style the tab control wears.
        static void Equip(Element Tab, Bool Closable, Text Style);

        /// \brief Moves the selection of a tab control by an offset, wrapping past either end, and focuses the new tab.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Self   The tab control to step.
        /// \param Offset The number of tabs to move by, negative to move back.
        static void Step(Ref<Interface> Owner, Handle Self, SInt32 Offset);

        /// \brief Runs a tab control, where a click or arrow key on a tab shows its page and a close click closes it.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it.
        /// \param Self    The tab control hearing the event.
        /// \return `true` if the event was a click or arrow key on a tab or a close click, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);
    };
}