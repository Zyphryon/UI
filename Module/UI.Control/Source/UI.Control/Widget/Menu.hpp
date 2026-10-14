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
    /// \brief Represents the state of a menu, with its style, what it was opened over, the item lit.
    struct Roster final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The name of the style the menu wears, its parts wearing it followed by theirs.
        Str    Style;

        /// The element the menu was opened over, as the item of a list a right press landed on.
        Handle Subject;

        /// The item lit, by the keyboard or the pointer, or one naming nothing.
        Handle Active;

        /// The item the pointer rested on at the last update, so the keyboard keeps what it lit until it moves.
        Handle Hovered;

        /// The control that held the keyboard before the menu took it.
        Handle Return;

        /// `true` when left and right, once the menu has no use for them, go on to what opened it.
        Bool   Passes = false;
    };

    /// \brief Represents a check item, which turns on a click and leaves its menu open, as a setting being tried does.
    struct Toggle final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;
    };

    /// \brief Represents the parts of a menu item given an icon, so its text is found past the icon standing before it.
    struct Option final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The label showing the item's text.
        Handle Label;

        /// The label showing the item's icon, as a glyph of an icon font baked into the text's typeface.
        Handle Icon;
    };

    /// \brief Represents a control that opens a menu under itself when clicked, as a title of a menu bar or a dropdown.
    struct Pulldown final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The root of the menu it opens.
        Handle Menu;
    };

    /// \brief Represents an item of a menu that opens another menu beside it.
    struct Branch final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The root of the menu it opens.
        Handle Submenu;
    };

    /// \brief Represents a menu, a popup of items, headings and separators where each item holds text.
    class Menu final
    {
    public:

        /// \brief Creates a menu, closed.
        ///
        /// \param Opener The element it is opened from, which hears its events.
        /// \param Style  The name of the style it wears.
        /// \return The root of the menu's pane.
        static Element Create(Element Opener, Text Style = "Menu");

        /// \brief Creates a menu from a layout node, with `Items` as text, `"-"` or objects, opened from its holder.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element it is opened from.
        /// \param Node   The layout node the menu is read from.
        /// \return The root of the menu's pane, closed.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Reads a node's `ContextMenu`, which is a list of items.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Target The element made from the node, which a right press opens the menu over.
        /// \param Node   The layout node that may hold the field.
        static void Read(Ref<Markup> Reader, Element Target, ConstRef<JsonValue> Node);

        /// \brief Adds the items a layout writes to a menu.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Target The menu the items are added to.
        /// \param Items  The list of items, as the layout writes them.
        static void Fill(Ref<Markup> Reader, Element Target, ConstRef<JsonValue> Items);

        /// \brief Adds an item to a menu.
        ///
        /// \param Target   The menu the item is added to.
        /// \param Content  The text of the item.
        /// \param Action   The name of the action picking it runs, or empty for none.
        /// \param Shortcut The text shown beside it, as the key doing the same, or empty for none.
        /// \return The item, which is disabled as any element is.
        static Element AddItem(Element Target, Text Content, Text Action = Text(), Text Shortcut = Text());

        /// \brief Adds an item that picking turns on or off, showing a check mark while it is on.
        ///
        /// \param Target   The menu the item is added to.
        /// \param Content  The text of the item.
        /// \param Checked  The flag, `true` to start it on, `false` to start it off.
        /// \param Action   The name of the action picking it runs, after it turned, or empty for none.
        /// \param Shortcut The text shown beside it, as the key doing the same, or empty for none.
        /// \return The item, which is `Checked` while on and reports \ref Signal::kChange each time picking turns it.
        static Element AddCheck(Element Target, Text Content, Bool Checked = false, Text Action = Text(),
                                Text Shortcut = Text());

        /// \brief Adds a heading over the items that follow it, which the pointer and the keyboard pass over.
        ///
        /// \param Target  The menu the heading is added to.
        /// \param Content The text of the heading, as the name of a group of items.
        /// \return The heading, which wears the menu's style followed by `.Heading`.
        static Element AddHeading(Element Target, Text Content);

        /// \brief Adds a line between the items above and below it.
        ///
        /// \param Target The menu the line is added to.
        /// \return The separator, which wears the menu's style followed by `.Separator`.
        static Element AddSeparator(Element Target);

        /// \brief Adds an item opening another menu beside it.
        ///
        /// \param Target  The menu the item is added to.
        /// \param Content The text of the item.
        /// \return The menu it opens, wearing the same style.
        static Element AddSubmenu(Element Target, Text Content);

        /// \brief Changes the text of an item.
        ///
        /// \param Item    The item whose text changes.
        /// \param Content The text shown on it.
        static void SetText(Element Item, Text Content);

        /// \brief Changes the icon standing before the text of an item, giving it one the first time.
        ///
        /// \param Item    The item whose icon changes.
        /// \param Content The text of the icon, as a glyph of an icon font baked into the text's typeface.
        static void SetIcon(Element Item, Text Content);

        /// \brief Turns an item made by \ref AddCheck on or off, without reporting it.
        ///
        /// \param Item    The item to turn.
        /// \param Checked The flag, `true` to turn it on, `false` to turn it off.
        static void SetChecked(Element Item, Bool Checked);

        /// \brief Checks whether an item is turned on.
        ///
        /// \param Item The item to look at.
        /// \return `true` if it is, `false` otherwise.
        static Bool IsChecked(Element Item);

        /// \brief Makes a menu the context menu of an element, opened by a right press on it or on what it holds.
        ///
        /// \param Target The element a right press opens the menu over.
        /// \param Menu   The menu it opens.
        static void Attach(Element Target, Element Menu);

        /// \brief Opens a menu at a point, taking the keyboard.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Target  The menu to open.
        /// \param At      The point its top left corner opens at, in pixels of the target.
        /// \param Subject The element it is opened over, or one naming nothing.
        static void Open(Ref<Interface> Owner, Handle Target, Vector2 At, Handle Subject = Handle());

        /// \brief Opens a menu under an element, on screen and taking the keyboard; the element is the \ref GetSubject.
        ///
        /// \param Owner    The interface it stands in.
        /// \param Target   The menu to open.
        /// \param Anchor   The element it opens under, lined up with its left edge.
        /// \param Wide     The flag, `true` to make it as wide as the element, `false` to make it as wide as its items.
        /// \param Keyboard The flag, `true` to light its first item, as the keyboard opens it, `false` to light none.
        static void Drop(Ref<Interface> Owner, Handle Target, Handle Anchor, Bool Wide = false, Bool Keyboard = false);

        /// \brief Closes a menu and every submenu open from it, without picking anything.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The menu to close.
        static void Close(Ref<Interface> Owner, Handle Target);

        /// \brief Closes the menus a menu was reached through and their submenus, returning the keyboard to its holder.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The menu, or a submenu, of the chain to close.
        static void Dismiss(Ref<Interface> Owner, Handle Target);

        /// \brief Gets what a menu was opened over, from any part of it or of its submenus.
        ///
        /// \param Part The part, as the item an action names.
        /// \return The element, or one naming nothing.
        static Element GetSubject(Element Part);

    private:

        /// \brief Runs a menu, where opening takes the keyboard, a click on an item picks it, and keys move through it.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching the menu.
        /// \param Self    The menu the handler governs.
        /// \return `true` if the event was the menu's, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Answers a key pressed while a menu holds the keyboard.
        ///
        /// \param Owner The interface it stands in.
        /// \param Key   The key pressed.
        /// \param Self  The menu holding the keyboard.
        /// \return `true` if the key moved through, picked or closed it, `false` otherwise.
        static Bool OnKey(Ref<Interface> Owner, ZyInput::Key Key, Handle Self);

        /// \brief Turns an item made by \ref AddCheck when it is clicked, reporting the change before its action runs.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching the item.
        /// \param Self    The item the handler governs.
        /// \return `false` always, so the click goes on to the item's action and closes its menu.
        static Bool OnCheck(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Lights the item the pointer rests on, opening its submenu and closing the others, while open.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The menu the pointer moves over.
        /// \return `true` if it opened or closed a submenu, `false` otherwise.
        static Bool Track(Ref<Interface> Owner, Handle Self);

        /// \brief Picks an item, opening its submenu or closing every menu it was reached through.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The menu holding the item.
        /// \param Item  The item picked.
        static void Choose(Ref<Interface> Owner, Handle Self, Handle Item);

        /// \brief Opens the submenu of an item beside it.
        ///
        /// \param Owner    The interface it stands in.
        /// \param Item     The item whose submenu opens.
        /// \param Keyboard The flag, `true` to give the submenu the keyboard and light its first item, `false` to show.
        static void Expand(Ref<Interface> Owner, Handle Item, Bool Keyboard);

        /// \brief Closes every submenu open from the items of a menu, but the one an item keeps open.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The menu whose submenus close.
        /// \param Kept  The item whose submenu stays open, or one naming nothing to close them all.
        /// \return `true` if a submenu closed, `false` otherwise.
        static Bool Collapse(Ref<Interface> Owner, Handle Self, Handle Kept);

        /// \brief Lights an item of a menu, letting the one lit before go.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The menu whose item is lit.
        /// \param Item  The item, or one naming nothing to light none.
        static void Light(Ref<Interface> Owner, Handle Self, Handle Item);

        /// \brief Lights the next item that can be picked, up or down, wrapping past either end.
        ///
        /// \param Owner     The interface it stands in.
        /// \param Self      The menu whose items are stepped through.
        /// \param Direction The direction, one to go down or minus one to go up.
        static void Step(Ref<Interface> Owner, Handle Self, SInt32 Direction);

        /// \brief Closes a menu and gives the keyboard back to the menu it was opened from, or to what held it before.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The menu to close.
        static void Retreat(Ref<Interface> Owner, Handle Self);

        /// \brief Gets the menu a submenu was opened from.
        ///
        /// \param Owner The tree it stands in.
        /// \param Self  The submenu to look up from.
        /// \return The parent menu, or one naming nothing for a menu opened on its own.
        static Handle GetParentMenu(ConstRef<Tree> Owner, Handle Self);

        /// \brief Gets the menu opened on its own that a chain of submenus leads back to.
        ///
        /// \param Owner The tree it stands in.
        /// \param Self  The menu or submenu to climb from.
        /// \return The first menu of the chain, the menu itself when it was opened on its own.
        static Handle GetTopMenu(ConstRef<Tree> Owner, Handle Self);

        /// \brief Checks whether an item can be picked, a control in the menu neither hidden nor disabled.
        ///
        /// \param Owner The tree it stands in.
        /// \param Item  The item to check.
        /// \return `true` if it can, `false` otherwise.
        static Bool IsPickable(ConstRef<Tree> Owner, Handle Item);

        /// \brief Adds one item a layout writes to a menu.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Target The menu the item is added to.
        /// \param Entry  The item as the layout writes it, which is text, `"-"` for a separator, or an object.
        static void Add(Ref<Markup> Reader, Element Target, ConstRef<JsonValue> Entry);

        /// \brief Gets the name of the style a menu's parts are named after, from any part of it.
        ///
        /// \param Part The menu or a part of it, as an item.
        /// \return The name of the menu's style, copied, since making a part may move the store its facet stands in.
        static Str GetStyle(Element Part);
    };
}