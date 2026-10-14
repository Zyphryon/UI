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
    /// \brief Specifies where a panel of a dock stands.
    enum class Side : UInt8
    {
        Left,            ///< In the region down the dock's left edge.
        Right,           ///< In the region down the dock's right edge.
        Top,             ///< In the region along the dock's top edge.
        Bottom,          ///< In the region along the dock's bottom edge.
        Center,          ///< In the region filling the middle.
        Float,           ///< In a window of its own, over everything.
    };

    /// \brief Represents a panel of a dock, with the name and title it goes by, what it holds, and where it stands.
    struct Berth final
    {
        /// The name it is found and saved by.
        Str     Name;

        /// The text on its tab or its window's title bar, which may bind values or be the key of a text of the lexicon.
        Str     Title;

        /// The panel holding what it shows, which moves from a tab's page to a window and back.
        Handle  Content;

        /// The window it floats in, or one naming nothing while docked.
        Handle  Window;

        /// The side it stands at.
        Side    Where = Side::Center;

        /// The side it last stood docked at, which closing its window docks it back to.
        Side    Home  = Side::Center;

        /// The side it moves to at the next update, once \ref Due.
        Side    Goal  = Side::Center;

        /// The top left corner of its window, where it stands or is to stand, in units of the interface.
        Vector2 Spot;

        /// The size of its window, in units of the interface.
        Vector2 Size;

        /// `true` when it moves to \ref Goal at the next update.
        Bool    Due   = false;

        /// `true` to dock it again even in the region it stands in, so a layout read back keeps its order of tabs.
        Bool    Again = false;

        /// `true` to show its tab over the others of its region once it is docked.
        Bool    Front = false;
    };

    /// \brief Represents a dock, with its panels, the regions they are docked in, and the drag carrying one.
    struct Harbor final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The panels, in the order they were added or a layout read back lists them.
        Sequence<Berth>  Berths;

        /// The tab control of each region, by \ref Side.
        Sequence<Handle> Regions;

        /// The count of tabs of each region when the dock last looked, by \ref Side.
        Sequence<UInt32> Counts;

        /// The splitter of the top, middle and bottom.
        Handle           Outer;

        /// The splitter of the left, centre and right, in the middle of the other.
        Handle           Middle;

        /// The root of the pane showing where a panel carried would dock, or one naming nothing.
        Handle           Overlay;

        /// The drop target of each region in that pane, by \ref Side.
        Sequence<Handle> Targets;

        /// The room lit in that pane where the panel would dock.
        Handle           Preview;

        /// The tab dragged out of its region, kept until the pointer lets go, or one naming nothing.
        Handle           Lifted;

        /// The panel the dragged tab's drag carries, or one naming nothing.
        Handle           Carried;

        /// `true` when the lifted tab is taken away at the next update.
        Bool             Loose = false;

        /// The style the dock wears, its parts wearing it followed by theirs.
        Str              Style;
    };

    /// \brief Represents a dock of five regions, each a \ref Tabs split by \ref Splitter bars, with floating panels.
    class Dock final
    {
    public:

        /// The size of a drop target, in units of the interface.
        static constexpr Real32 kTarget  = 40.0f;

        /// The room between two drop targets, in units of the interface.
        static constexpr Real32 kSpacing = 8.0f;

        /// The distance a tab is dragged past its region's tabs before its panel floats, in units of the interface.
        static constexpr Real32 kTear    = 16.0f;

        /// The event reported when a panel is docked, floated or picked, its `Content` naming the panel.
        static constexpr UInt64 kDock    = Identify("Dock");

    public:

        /// \brief Creates a dock holding no panel.
        ///
        /// \param Parent The element the dock is added to.
        /// \param Style  The name of the style the dock wears.
        /// \return The dock, which reports \ref Dock::kDock, naming the panel, when the pointer docks one.
        static Element Create(Element Parent, Text Style = "Dock");

        /// \brief Creates a dock from a layout node, with its `Panels` and the `Sizes` of its four edge regions.
        ///
        /// \param Reader The markup reading the layout, which builds the children of each panel.
        /// \param Parent The element the dock is added to.
        /// \param Node   The layout node the dock is read from.
        /// \return The dock, holding every panel the node lists.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Adds a panel, docked as the last tab of a region or floating in a window of its own.
        ///
        /// \param Owner The dock the panel is added to.
        /// \param Name  The name it is found and saved by, which no other panel of the dock has.
        /// \param Title The text on its tab or title bar, which may bind values or be the key of a text of the lexicon.
        /// \param Where The side the panel stands at.
        /// \return The panel, to hold what it shows, or the panel already named so.
        static Element AddPanel(Element Owner, Text Name, Text Title, Side Where = Side::Center);

        /// \brief Moves a panel to a region or floats it in a window, at the next update when called in a handler.
        ///
        /// \param Owner    The dock holding the panel.
        /// \param Name     The name of the panel to move.
        /// \param Where    The side the panel moves to.
        /// \param Position The top left corner of its window when it floats, in units of the interface.
        static void Move(Element Owner, Text Name, Side Where, Vector2 Position = Vector2(80.0f, 80.0f));

        /// \brief Gets where a panel stands.
        ///
        /// \param Owner The dock holding the panel.
        /// \param Name  The name of the panel looked for.
        /// \return The side it stands at, or \ref Side::Center for one the dock does not hold.
        static Side GetSide(Element Owner, Text Name);

        /// \brief Gets a panel by its name.
        ///
        /// \param Owner The dock holding the panel.
        /// \param Name  The name of the panel looked for.
        /// \return The panel, or one naming nothing when the dock holds none so named.
        static Element GetPanel(Element Owner, Text Name);

        /// \brief Gets the window a panel floats in.
        ///
        /// \param Owner The dock holding the panel.
        /// \param Name  The name of the panel looked for.
        /// \return The window, or one naming nothing while the panel is docked.
        static Element GetWindow(Element Owner, Text Name);

        /// \brief Gets the tab control of a region.
        ///
        /// \param Owner The dock the region belongs to.
        /// \param Where The region, any side but \ref Side::Float.
        /// \return The tab control holding the region's panels.
        static Element GetRegion(Element Owner, Side Where);

        /// \brief Gets how many panels a dock holds.
        ///
        /// \param Owner The dock whose panels are counted.
        /// \return The count of panels, floating ones included.
        static UInt32 GetCount(Element Owner);

        /// \brief Gets where the drop target of a region stands while a panel is carried.
        ///
        /// \param Owner The dock the target stands over.
        /// \param Where The region, any side but \ref Side::Float.
        /// \return The target, in units of the interface, in the middle of the dock as it was last laid out.
        static Rect GetTarget(Element Owner, Side Where);

        /// \brief Gets the root of the pane showing the drop targets.
        ///
        /// \param Owner The dock the pane stands over.
        /// \return The root, or one naming nothing while no panel is carried.
        static Element GetOverlay(Element Owner);

        /// \brief Writes the layout out as JSON, with edge region sizes, each panel's tab order and floating frames.
        ///
        /// \param Owner The dock whose layout is written.
        /// \return The JSON text, which \ref Load reads back.
        static Str Save(Element Owner);

        /// \brief Reads a layout written by \ref Save back, skipping and logging panels it names that the dock lacks.
        ///
        /// \param Owner  The dock whose panels are moved.
        /// \param Source The JSON text \ref Save wrote.
        /// \param Error  The variable receiving why the text cannot be read.
        /// \return `true` if it was read, `false` otherwise.
        static Bool Load(Element Owner, Text Source, Ref<Str> Error);

    private:

        /// \brief Runs a dock, where a tab dragged out of a region floats its panel and a drop on a target docks it.
        ///
        /// \param Owner   The interface the dock stands in.
        /// \param Message The event the dock receives.
        /// \param Self    The dock receiving the event.
        /// \return `true` if the event dragged, floated or docked a panel, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Runs a floating window, showing drop targets while dragged and docking its panel where it is let go.
        ///
        /// \param Owner   The interface the window stands in.
        /// \param Message The event the window receives.
        /// \param Self    The floating window receiving the event.
        /// \return `true` if the event moved, held, docked or closed it, `false` otherwise.
        static Bool OnFloat(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Moves every panel due, takes the lifted tab away once let go, and hides each empty region.
        ///
        /// \param Owner The interface the dock stands in.
        /// \param Self  The dock to settle.
        /// \return `true` if a panel moved or a region changed, `false` otherwise.
        static Bool OnRealize(Ref<Interface> Owner, Handle Self);

        /// \brief Moves every panel due to move, and shows each region holding a panel and hides the others.
        ///
        /// \param Root The dock to settle, which must not be walked.
        /// \return `true` if anything moved, `false` otherwise.
        static Bool Settle(Element Root);

        /// \brief Moves a panel where it is due to go, out of the tab or window holding it.
        ///
        /// \param Root  The dock holding the panel.
        /// \param Index The index of the panel to move.
        static void Place(Element Root, UInt Index);

        /// \brief Opens the window a panel floats in.
        ///
        /// \param Root  The dock the panel belongs to.
        /// \param Entry The panel the window is opened for.
        /// \return The window, standing at the panel's spot at the panel's size.
        static Element Launch(Element Root, ConstRef<Berth> Entry);

        /// \brief Binds the title of a tab or window to a panel's title, when it names values or a lexicon text.
        ///
        /// \param Holder The tab, or the window, whose title is bound.
        /// \param Title  The panel's title, which may bind values or be the key of a text of the lexicon.
        /// \param Floats The flag, `true` to bind a window's title, `false` a tab's.
        static void Entitle(Element Holder, Text Title, Bool Floats);

        /// \brief Adds the tab of a panel as the last of a region, its title bound to the panel's.
        ///
        /// \param Region The tab control of the region the panel docks in.
        /// \param Title  The panel's title, which may bind values or be the key of a text of the lexicon.
        /// \param Style  The style the dock wears, which the tab wears followed by `.Region`.
        /// \return The tab's page, to hold the panel.
        static Element Moor(Element Region, Text Title, Text Style);

        /// \brief Gives the left, right, top and bottom regions the sizes a list names, in that order.
        ///
        /// \param Root  The dock whose regions are sized.
        /// \param Sizes The list of sizes, in units of the interface.
        static void Resize(Element Root, ConstRef<JsonValue> Sizes);

        /// \brief Shows the drop targets over a dock, lighting the one under the pointer and the room it would dock in.
        ///
        /// \param Root  The dock the targets stand over.
        /// \param Point The pointer, in units of the interface.
        static void Hover(Element Root, Vector2 Point);

        /// \brief Places a part of the drop target pane at a room, standing apart from the flow.
        ///
        /// \param Target The part placed.
        /// \param Room   The room it takes, in units of the interface.
        /// \param Origin The corner its parent stands at, which the room is placed from.
        static void Pin(Element Target, Rect Room, Vector2 Origin);

        /// \brief Hides the drop targets.
        ///
        /// \param Root The dock whose targets are hidden.
        static void Clear(Element Root);

        /// \brief Lets a carried panel go, docked in the region whose target is under the pointer, else left floating.
        ///
        /// \param Owner The interface the dock stands in.
        /// \param Root  The dock holding the panel.
        /// \param Index The index of the carried panel.
        /// \param Point The pointer, in units of the interface.
        static void Land(Ref<Interface> Owner, Element Root, UInt Index, Vector2 Point);

        /// \brief Sends a panel to a region at the next update, as the tab over the others, reporting \ref Dock::kDock.
        ///
        /// \param Owner The interface the dock stands in.
        /// \param Root  The dock holding the panel.
        /// \param Index The index of the panel to send.
        /// \param Goal  The side the panel docks at.
        static void Steer(Ref<Interface> Owner, Element Root, UInt Index, Side Goal);

        /// \brief Finds the drop target under a point.
        ///
        /// \param Frame The dock's frame, in units of the interface.
        /// \param Point The point looked under, in units of the interface.
        /// \return The index of the region, as \ref Side counts them, or below nought over none.
        static SInt32 Aim(Rect Frame, Vector2 Point);

        /// \brief Gets where a region's drop target stands, the five of them gathered in the middle of the dock.
        ///
        /// \param Frame The dock's frame, in units of the interface.
        /// \param Where The region whose target is placed.
        /// \return The target, in units of the interface.
        static Rect Mark(Rect Frame, Side Where);

        /// \brief Gets the room a panel docked in a region would take, shown while its target is under the pointer.
        ///
        /// \param Frame The dock's frame, in units of the interface.
        /// \param Where The region the panel would dock in.
        /// \return The room, in units of the interface.
        static Rect Shade(Rect Frame, Side Where);

        /// \brief Finds the dock a floating window belongs to.
        ///
        /// \param Owner  The interface the window stands in.
        /// \param Window The floating window whose dock is looked for.
        /// \param Index  The variable receiving the index of the panel floating in it.
        /// \return The dock, or one naming nothing.
        static Element Seek(Ref<Interface> Owner, Handle Window, Ref<UInt> Index);

        /// \brief Finds a panel by its name.
        ///
        /// \param Own  The dock's facet, holding its panels.
        /// \param Name The name of the panel looked for.
        /// \return The index of the panel, or below nought when the dock holds none so named.
        static SInt32 Find(ConstRef<Harbor> Own, Text Name);

        /// \brief Finds the panel whose page a tab shows.
        ///
        /// \param Root The dock holding the panel.
        /// \param Tab  The tab whose panel is looked for.
        /// \return The index of the panel, or below nought for a tab of no region of the dock.
        static SInt32 Find(Element Root, Element Tab);
    };
}