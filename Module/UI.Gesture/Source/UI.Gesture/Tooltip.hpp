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

#include "Filler.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Provides tooltips, a line of text or a template an element shows near the pointer once it rests on it.
    class Tooltip final
    {
    public:

        /// The band tooltips are shown in.
        static constexpr Layer kLayer = { 192, Layer::Trait::Overlay };

    public:

        /// \brief Gives an element a tooltip showing a line of text.
        ///
        /// \param Target  The element given the tooltip.
        /// \param Content The line of text the tooltip shows.
        static void SetText(Element Target, Text Content);

        /// \brief Gives an element a tooltip its own code fills.
        ///
        /// \param Target The element given the tooltip.
        /// \param Fill   The code, handed the tooltip's root and the element.
        static void SetTemplate(Element Target, Filler Fill);

        /// \brief Gives an element a tooltip made from a template.
        ///
        /// \param Target   The element given the tooltip.
        /// \param Reader   The markup knowing the template, which must outlive the element.
        /// \param Template The name of the template the tooltip is made from.
        static void SetTemplate(Element Target, Ref<Markup> Reader, Text Template);

        /// \brief Takes an element's tooltip away.
        ///
        /// \param Target The element whose tooltip goes.
        static void Clear(Element Target);

        /// \brief Gets the text an element's tooltip shows.
        ///
        /// \param Target The element whose tooltip is read.
        /// \return The text, or empty when it shows none.
        static Text GetText(Element Target);

        /// \brief Reads the `Tooltip` a node may give, a template's name or a line of text that may bind.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Target The element made from the node.
        /// \param Node   The layout node the field is read from.
        static void Read(Ref<Markup> Reader, Element Target, ConstRef<JsonValue> Node);

        /// \brief Gets the record the tooltip of an element reads, for the binder to move with its row.
        ///
        /// \param Target The element whose tooltip template reads the record.
        /// \return The steps of the record, or `nullptr` when the element shows no tooltip.
        static Ptr<Path> GetWithin(Element Target);

        /// \brief Runs tooltips on an interface, once however many times it is declared.
        ///
        /// \param Target The interface tooltips run on.
        static void Declare(Ref<Interface> Target);

        /// \brief Gets the root of the pane the tooltips are shown in.
        ///
        /// \param Owner The interface the tooltips run on.
        /// \return The root, or one naming nothing before the first tooltip was shown.
        static Handle GetPane(Ref<Interface> Owner);

        /// \brief Changes how long the pointer rests on an element before its tooltip shows.
        ///
        /// \param Owner   The interface the tooltips run on.
        /// \param Seconds The time the pointer must rest, in seconds.
        static void SetDelay(Ref<Interface> Owner, Real32 Seconds);

        /// \brief Hides the tooltip until the pointer leaves what it rests on, as a press or the wheel does.
        ///
        /// \param Owner The interface the tooltips run on.
        static void Silence(Ref<Interface> Owner);

        /// \brief Fills the tooltip shown again from its element, as after the wheel stepped what it shows.
        ///
        /// \param Owner The interface the tooltips run on.
        static void Refresh(Ref<Interface> Owner);

    private:

        /// How far a tooltip stands from the pointer, in units, clear of the cursor's arrow.
        static constexpr Real32 kGap = 16.0f;

        /// \brief Holds the tooltips of one interface, as what the pointer rests on and what shows.
        struct Session final
        {
            /// The nearest element showing a tooltip the pointer rests on.
            Handle Hinted;

            /// The element a press or the wheel hid the tooltip of, until the pointer leaves it.
            Handle Hushed;

            /// The element the tooltip shows for, or one naming nothing while none shows.
            Handle Shown;

            /// The root of the pane the tooltips are shown in.
            Handle Pane;

            /// How long the pointer rested, in seconds.
            Real32 Clock = 0.0f;

            /// How long it must rest before the tooltip shows, in seconds.
            Real32 Delay = 0.5f;
        };

        /// \brief Fills a tooltip with the line of text its element gives.
        ///
        /// \param Holder The root of the tooltip.
        /// \param Source The element whose text the tooltip shows.
        static void Write(Element Holder, Element Source);

        /// \brief Fills a tooltip from the template its element names.
        ///
        /// \param Holder The root of the tooltip.
        /// \param Source The element naming the template.
        static void Spawn(Element Holder, Element Source);

        /// \brief Hides the tooltip at a press.
        ///
        /// \param Owner The interface the press went down on.
        static void OnPressed(Ref<Interface> Owner, ZyInput::Button);

        /// \brief Hides the tooltip at a turn of the wheel no control took, which scrolls or does nothing.
        ///
        /// \param Owner   The interface the wheel turned over.
        /// \param Stopped The flag, `true` when a control took the turn, which leaves the tooltip up.
        static void OnTurned(Ref<Interface> Owner, Bool Stopped);

        /// \brief Follows what the pointer rests on, a tooltip of another element starting its wait over.
        ///
        /// \param Owner The interface the pointer moved over.
        static void OnPicked(Ref<Interface> Owner);

        /// \brief Counts how long the pointer rests on an element with a tooltip, showing it once long enough.
        ///
        /// \param Owner The interface the tooltips run on.
        /// \param Delta The time since the last update, in seconds.
        static void OnLinger(Ref<Interface> Owner, Real32 Delta);

        /// \brief Finds the nearest element holding another, or the element itself, that shows a tooltip.
        ///
        /// \param Owner   The interface the element stands in.
        /// \param Element The element the pointer rests on, where the search starts.
        /// \return The element showing a tooltip, or one naming nothing.
        static Handle Hinted(Ref<Interface> Owner, Handle Element);

        /// \brief Empties the tooltip's pane and fills it from an element's tooltip.
        ///
        /// \param Owner  The interface the tooltips run on.
        /// \param Run    The tooltips, whose pane is filled.
        /// \param Source The element whose tooltip fills the pane, which must show one.
        static void Fill(Ref<Interface> Owner, Ref<Session> Run, Handle Source);

        /// \brief Shows the tooltip of the element the pointer rests on, near the pointer.
        ///
        /// \param Owner The interface the tooltips run on.
        /// \param Run   The tooltips, whose hinted element shows its tooltip.
        static void Tip(Ref<Interface> Owner, Ref<Session> Run);

        /// \brief Hides the tooltip.
        ///
        /// \param Owner The interface the tooltips run on.
        /// \param Run   The tooltips, whose shown tooltip hides.
        static void Hush(Ref<Interface> Owner, Ref<Session> Run);
    };
}