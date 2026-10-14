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

#include "Facet/Droppable.hpp"
#include "Filler.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Facet.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Datum.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Provides drag and drop to any element, where a source carrying a kind is dropped on a target.
    class Drag final
    {
    public:

        /// The pointer picked the control up and carries it.
        static constexpr UInt64 kStart = Identify("DragStart");

        /// A drag the control accepts came over it; `Other` names what is carried.
        static constexpr UInt64 kEnter = Identify("DragEnter");

        /// A drag the control accepted left it, or was let go elsewhere; `Other` names what is carried.
        static constexpr UInt64 kLeave = Identify("DragLeave");

        /// A drag the control accepts was let go over it; `Other` names what is carried.
        static constexpr UInt64 kDrop  = Identify("DragDrop");

        /// The drag carrying the control ended; `Other` names where it was dropped, or nothing when it was not.
        static constexpr UInt64 kEnd   = Identify("DragEnd");

        /// The band ghosts are carried in.
        static constexpr Layer  kLayer = { 224, Layer::Trait::Overlay };

    public:

        /// \brief Lets the pointer pick an element up.
        ///
        /// \param Target  The element the pointer may pick up.
        /// \param Kind    The kind of thing it is, which targets accept it by, or empty for none.
        /// \param Payload The value it carries to whatever it is dropped on.
        static void SetSource(Element Target, Text Kind = Text(), ConstRef<Datum> Payload = Datum());

        /// \brief Changes the value an element carries when it is picked up, making it a source when it is not.
        ///
        /// \param Target  The element whose carried value changes.
        /// \param Payload The value it carries from now on.
        static void SetPayload(Element Target, ConstRef<Datum> Payload);

        /// \brief Gets the value an element carries.
        ///
        /// \param Target The element, which may be a target's `Other` while a drag reaches it.
        /// \return The value, holding nothing when the element is no source.
        static Datum GetPayload(Element Target);

        /// \brief Fills the ghost of a source with code rather than a copy of it.
        ///
        /// \param Target The source whose ghost the code fills.
        /// \param Fill   The code, handed the ghost and the source.
        static void SetGhost(Element Target, Filler Fill);

        /// \brief Creates the ghost of a source from a template rather than a copy of it.
        ///
        /// \param Target   The source whose ghost the template makes.
        /// \param Reader   The markup knowing the template, which must outlive the source.
        /// \param Template The name of the template the ghost is made from.
        static void SetGhost(Element Target, Ref<Markup> Reader, Text Template);

        /// \brief Lets drags be dropped on an element.
        ///
        /// \param Target The element drags may be dropped on.
        /// \param Kinds  The kinds it accepts, separated by commas as `Item, Gem`, or empty to accept every kind.
        static void SetTarget(Element Target, Text Kinds = Text());

        /// \brief Checks whether a target accepts what a source is.
        ///
        /// \param Target The element drags are dropped on.
        /// \param Source The element the pointer carries.
        /// \return `true` if it does, `false` otherwise.
        static Bool Accepts(Element Target, Element Source);

        /// \brief Reads the `Drag`, `DragKind`, `DragTemplate` and `Drop` fields a node may give.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Target The element made from the node.
        /// \param Node   The layout node the fields are read from.
        static void Read(Ref<Markup> Reader, Element Target, ConstRef<JsonValue> Node);

        /// \brief Gets the record the ghost of a source reads, for the binder to move with its row.
        ///
        /// \param Target The source whose ghost template reads the record.
        /// \return The steps of the record, or `nullptr` when the element is no source.
        static Ptr<Path> GetWithin(Element Target);

        /// \brief Gets the state of an element the pointer carries, declaring it the first time.
        ///
        /// \return The state, set on a source while it is carried.
        static State Dragging();

        /// \brief Gets the state of a target under a drag it accepts, declaring it the first time.
        ///
        /// \return The state, set on a target while a drag it accepts is over it.
        static State DropHover();

        /// \brief Runs drag and drop on an interface, once however many times it is declared.
        ///
        /// \param Target The interface drag and drop runs on.
        static void Declare(Ref<Interface> Target);

        /// \brief Gets the element the pointer carries.
        ///
        /// \param Owner The interface the drag runs on.
        /// \return The element, or one naming nothing while no drag runs.
        static Handle GetDragged(Ref<Interface> Owner);

        /// \brief Gets the element a drag would land on if it were let go now.
        ///
        /// \param Owner The interface the drag runs on.
        /// \return The element, or one naming nothing.
        static Handle GetTarget(Ref<Interface> Owner);

        /// \brief Gets the root of the pane holding the ghost a drag carries.
        ///
        /// \param Owner The interface the drag runs on.
        /// \return The root, or one naming nothing while no drag runs.
        static Handle GetGhost(Ref<Interface> Owner);

        /// \brief Ends the running drag without dropping it, as the escape key does.
        ///
        /// \param Owner The interface the drag runs on.
        static void Cancel(Ref<Interface> Owner);

    private:

        /// How far the pointer moves with a button held before a press on a source becomes a drag, in units.
        static constexpr Real32 kThreshold = 4.0f;

        /// The most elements a ghost copies, so dragging a large panel never copies all it holds.
        static constexpr UInt32 kMaxGhost  = 256;

        /// \brief Holds the drag of one interface, as what a press may pick up and what is carried where.
        struct Session final
        {
            /// The source a left press landed on, picked up once the pointer moves far enough.
            Handle  Candidate;

            /// Where that press went down, in units of the interface.
            Vector2 PressAt;

            /// The element carried, or one naming nothing while no drag runs.
            Handle  Dragged;

            /// The target lit under the drag.
            Handle  Target;

            /// The root of the pane holding the ghost.
            Handle  Ghost;

            /// Where on the source the pointer took hold of it.
            Vector2 Hold;
        };

        /// \brief Represents an element a ghost copies, and the element of the ghost the copy goes under.
        struct Fold final
        {
            /// The element of the source being copied.
            Handle From;

            /// The element of the ghost holding the copy.
            Handle Into;
        };

        /// \brief Fills the ghost of a source from the template it names.
        ///
        /// \param Holder The root of the ghost.
        /// \param Source The source naming the template.
        static void Spawn(Element Holder, Element Source);

        /// \brief Lets another button going down while something is carried let it go nowhere.
        ///
        /// \param Owner The interface the drag runs on.
        /// \return `true` if a drag ran, `false` otherwise.
        static Bool OnPress(Ref<Interface> Owner, ZyInput::Button);

        /// \brief Remembers the source a left press landed on, picked up once the pointer moves far enough.
        ///
        /// \param Owner  The interface the press went down on.
        /// \param Button The button that went down, of which only the left one picks anything up.
        static void OnPressed(Ref<Interface> Owner, ZyInput::Button Button);

        /// \brief Carries what is carried, or picks a source up once the pointer moved far enough.
        ///
        /// \param Owner The interface the pointer moved over.
        /// \return `true` if a drag ran on, `false` otherwise.
        static Bool OnMove(Ref<Interface> Owner);

        /// \brief Lets go of what is carried, on the target under the pointer.
        ///
        /// \param Owner  The interface the drag runs on.
        /// \param Button The button that came up, of which only the left one lets go.
        /// \return `true` if a drag ended, `false` otherwise.
        static Bool OnRelease(Ref<Interface> Owner, ZyInput::Button Button);

        /// \brief Lets escape let go of what is carried before anything holding the keyboard hears it.
        ///
        /// \param Owner The interface the drag runs on.
        /// \param Key   The key that went down, of which only escape lets go.
        /// \return `true` if a drag ended, `false` otherwise.
        static Bool OnKey(Ref<Interface> Owner, ZyInput::Key Key);

        /// \brief Finds the nearest element holding another, or the element itself, that the pointer can pick up.
        ///
        /// \param Owner   The interface the element stands in.
        /// \param Element The element the pointer is over, where the search starts.
        /// \return The element to carry, or one naming nothing.
        static Handle Carrier(Ref<Interface> Owner, Handle Element);

        /// \brief Finds the nearest element holding another, or the element itself, that accepts what is carried.
        ///
        /// \param Owner   The interface the element stands in.
        /// \param Run     The running drag, naming what is carried.
        /// \param Element The element the pointer is over, where the search starts.
        /// \return The target, or one naming nothing, as when the element is what is carried or stands in it.
        static Handle Receiver(Ref<Interface> Owner, ConstRef<Session> Run, Handle Element);

        /// \brief Starts carrying the source a press landed on, making the ghost that follows the pointer.
        ///
        /// \param Owner The interface the drag runs on.
        /// \param Run   The drag, whose candidate is picked up.
        static void Lift(Ref<Interface> Owner, Ref<Session> Run);

        /// \brief Moves the ghost of the running drag to the pointer.
        ///
        /// \param Owner The interface the drag runs on.
        /// \param Run   The drag, whose ghost is moved.
        static void Carry(Ref<Interface> Owner, ConstRef<Session> Run);

        /// \brief Finds the target under the pointer, lighting it and telling it and the one it left.
        ///
        /// \param Owner The interface the drag runs on.
        /// \param Run   The drag, whose lit target changes.
        static void Aim(Ref<Interface> Owner, Ref<Session> Run);

        /// \brief Ends the running drag, dropping what is carried on a target.
        ///
        /// \param Owner  The interface the drag runs on.
        /// \param Run    The drag, which is over once this returns.
        /// \param Target The target, or one naming nothing to drop it nowhere.
        static void Land(Ref<Interface> Owner, Ref<Session> Run, Handle Target);

        /// \brief Sends one of the drag's signals to an element, at the pointer.
        ///
        /// \param Owner  The interface the drag runs on.
        /// \param Kind   The signal sent, one of \ref kStart, \ref kEnter, \ref kLeave, \ref kDrop or \ref kEnd.
        /// \param Target The element hearing it.
        /// \param Other  The other element the drag involves, or one naming nothing.
        static void Tell(Ref<Interface> Owner, UInt64 Kind, Handle Target, Handle Other);

        /// \brief Copies an element and what it holds into the ghost, to the look the theme gives it.
        ///
        /// \param Owner  The interface the drag runs on.
        /// \param Source The element copied, with all it holds.
        /// \param Ghost  The root of the ghost.
        static void Copy(Ref<Interface> Owner, Handle Source, Handle Ghost);

        /// \brief Copies a facet of an element to another, when the element holds one.
        ///
        /// \param Owner The interface both elements stand in.
        /// \param From  The element whose facet is copied.
        /// \param Into  The element given the copy.
        template<IsFacet Type>
        static void Clone(Ref<Interface> Owner, Handle From, Handle Into);

        /// \brief Gives a copy an element's style, box, lasting states and the facets it draws.
        ///
        /// \param Owner The interface both elements stand in.
        /// \param From  The element imitated.
        /// \param Into  The copy in the ghost taking its look.
        static void Imitate(Ref<Interface> Owner, Handle From, Handle Into);

        /// \brief Reads the value a node gives a source to carry, written as it is.
        ///
        /// \param Value The value written, which is carried when it is a flag, a number or a text.
        /// \return The value carried, holding nothing for any other.
        static Datum Parse(ConstRef<JsonValue> Value);

        /// \brief Adds the kinds written in a text to those a target accepts.
        ///
        /// \param Accepts The target facet the kinds are added to.
        /// \param Kinds   The kinds, separated by commas.
        static void Accept(Ref<Droppable> Accepts, Text Kinds);
    };
}