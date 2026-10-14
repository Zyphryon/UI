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

#include "Dispatcher.hpp"
#include "Pointer.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Shortcut.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the keyboard of an interface, with the control holding it and the keys that move it.
    class Focus final
    {
    public:

        /// \brief Constructs a keyboard held by no control, over the elements of a tree.
        ///
        /// \param Owner      The tree whose controls take the keyboard, which must outlive it.
        /// \param Dispatcher The dispatcher its events go through.
        /// \param Pointer    The pointer, whose drag a key lets go of.
        Focus(Ref<Tree> Owner, Ref<Dispatcher> Dispatcher, Ref<Pointer> Pointer);

        /// \brief Moves the keyboard to a control, or takes it away.
        ///
        /// \param Element The control taking the keyboard, or one naming nothing to take it away.
        void Set(Handle Element);

        /// \brief Gets the control holding the keyboard.
        ///
        /// \return The control, or one naming nothing.
        ZY_INLINE Handle GetFocused() const
        {
            return mFocused;
        }

        /// \brief Moves the keyboard to the next control in layout order, wrapping past the last, as the tab key does.
        ///
        /// \return `true` if a control took the keyboard, `false` otherwise.
        Bool NextFocus();

        /// \brief Moves the keyboard to the previous control in layout order, wrapping past the first.
        ///
        /// \return `true` if a control took the keyboard, `false` otherwise.
        Bool PreviousFocus();

        /// \brief Moves the keyboard toward the nearest control one way on screen.
        ///
        /// \param Way The way on screen to move toward.
        /// \return `true` if a control took the move or the keyboard, `false` otherwise.
        Bool Navigate(Heading Way);

        /// \brief Clicks the control holding the keyboard, as enter, space or a pad's confirm button do.
        ///
        /// \return `true` if a control holds the keyboard, `false` otherwise.
        Bool Activate();

        /// \brief Backs out as escape or a pad's back button does.
        ///
        /// \return `true` if anything heard it or closed, `false` otherwise.
        Bool Cancel();

        /// \brief Scrolls every scrolling element holding an element so the element stands in its view.
        ///
        /// \note An element not laid out yet is kept, and brought into view by \ref Settle once it is.
        ///
        /// \param Element The element brought into view.
        void Reveal(Handle Element);

        /// \brief Brings into view an element asked for before it was laid out, once a layout has placed it.
        ///
        /// \return `true` when it was brought into view, `false` when none was waiting or it still waits.
        Bool Settle();

        /// \brief Checks whether a text field holds the keyboard, as a control taking typed text does.
        ///
        /// \return `true` if one does, `false` otherwise.
        Bool IsTyping() const;

        /// \brief Adds a shortcut, kept by the interface or by an element it goes with.
        ///
        /// \param Entry The shortcut.
        /// \param Owner The element keeping it, or one naming nothing to keep it for the interface.
        void AddShortcut(ConstRef<Shortcut> Entry, Handle Owner);

        /// \brief Removes every shortcut of a chord the interface or an element keeps.
        ///
        /// \param Key       The key of the chord.
        /// \param Modifiers The modifier keys of the chord.
        /// \param Owner     The element keeping them, or one naming nothing for the interface's own.
        void RemoveShortcut(ZyInput::Key Key, ZyInput::Modifier Modifiers, Handle Owner);

        /// \brief Presses a key, which the shortcuts hear first, then the focused control.
        ///
        /// \param Key       The key pressed.
        /// \param Modifiers The modifier keys held with it, as shift turns tab backward.
        /// \param Repeat    The flag, `true` when the key repeats as it is held, `false` when it went down.
        /// \return `true` if the key was taken or moved the keyboard, `false` otherwise.
        Bool OnKeyDown(ZyInput::Key Key, ZyInput::Modifier Modifiers = ZyInput::Modifier::None, Bool Repeat = false);

        /// \brief Types text, which the control holding the keyboard hears, unless a modal pane stands over it.
        ///
        /// \param Content The text typed.
        /// \return `true` if a control or a modal pane holds the keyboard, `false` otherwise.
        Bool OnKeyType(Text Content);

    private:

        /// The weight added to a control off the line of an arrow's move, so one in line always wins over a nearer one.
        static constexpr Real32 kBeam = 100000.0f;

        /// \brief Presses a key no shortcut hears, as a pad's moves, confirm and back do.
        ///
        /// \param Key       The key pressed.
        /// \param Modifiers The modifier keys held with it.
        /// \return `true` if a control or a modal pane holds the keyboard or the key moved it, `false` otherwise.
        Bool Press(ZyInput::Key Key, ZyInput::Modifier Modifiers);

        /// \brief Hands a key to the control holding the keyboard.
        ///
        /// \param Key       The key pressed.
        /// \param Modifiers The modifier keys held with it.
        /// \return `true` if a control or a modal pane holds the keyboard or the key moved it, `false` otherwise.
        Bool Deliver(ZyInput::Key Key, ZyInput::Modifier Modifiers);

        /// \brief Runs the first shortcut of a chord whose scope holds and whose code takes the key.
        ///
        /// \param Key       The key pressed.
        /// \param Modifiers The modifier keys held with it.
        /// \param Repeat    The flag, `true` when the key repeats as it is held.
        /// \return `true` if a shortcut took the key, `false` otherwise.
        Bool Answer(ZyInput::Key Key, ZyInput::Modifier Modifiers, Bool Repeat);

        /// \brief Checks whether a shortcut answers a chord, where its element's scope holds.
        ///
        /// \param Entry     The shortcut.
        /// \param Owner     The element keeping it, or one naming nothing for the interface's own.
        /// \param Key       The key pressed.
        /// \param Modifiers The modifier keys of shift, control and alt held with it.
        /// \param Repeat    The flag, `true` when the key repeats as it is held.
        /// \return `true` if it does, `false` otherwise.
        Bool Answers(ConstRef<Shortcut> Entry,
                     Handle             Owner,
                     ZyInput::Key       Key,
                     ZyInput::Modifier  Modifiers,
                     Bool               Repeat) const;

        /// \brief Runs a shortcut, taking the keyboard from a text field first when it asks.
        ///
        /// \param Entry     The shortcut, copied, since its code may add or remove shortcuts.
        /// \param Owner     The element keeping it, or one naming nothing to target the focused control.
        /// \param Key       The key pressed.
        /// \param Modifiers The modifier keys held with it.
        /// \return `true` if its code took the key, `false` otherwise.
        Bool Perform(Shortcut Entry, Handle Owner, ZyInput::Key Key, ZyInput::Modifier Modifiers);

        /// \brief Moves the keyboard as a key no control kept asks.
        ///
        /// \param Key       The key no control kept.
        /// \param Modifiers The modifier keys held with it.
        /// \param Target    The control that heard the key, the modal pane's root when one stands over the keyboard.
        /// \return `true` if the key moved, clicked, closed or scrolled anything, `false` otherwise.
        Bool Steer(ZyInput::Key Key, ZyInput::Modifier Modifiers, Handle Target);

        /// \brief Finds the root of the pane the keyboard moves within.
        ///
        /// \return The root, or one naming nothing when no pane holds such a control.
        Handle Arena();

        /// \brief Gathers the innermost controls of a pane the keyboard stops at, in layout order.
        ///
        /// \param Root   The root of the pane, which is never one of them.
        /// \param Output The receiver of the controls, added to those it held.
        void Gather(Handle Root, Ref<Sequence<Handle>> Output) const;

        /// \brief Moves the keyboard to the next or the previous control of the pane it moves within.
        ///
        /// \param Forward The flag, `true` to move to the next control, `false` to the previous one.
        /// \return `true` if a control took the keyboard, `false` otherwise.
        Bool Cycle(Bool Forward);

        /// \brief Moves the keyboard to the nearest control one way, looking in other panes when its own has none.
        ///
        /// \param Way The way on screen to move toward.
        /// \return `true` if a control took the keyboard, `false` otherwise.
        Bool Travel(Heading Way);

        /// \brief Finds the control nearest one way of a box among some, those in line with it weighing least.
        ///
        /// \param Candidates The controls that may take the keyboard, the focused one among them skipped.
        /// \param From       The box moved from, in pixels of the target.
        /// \param Way        The way on screen to move toward.
        /// \return The control, or one naming nothing when none stands that way.
        Handle Toward(ConstRef<Sequence<Handle>> Candidates, Rect From, Heading Way) const;

        /// \brief Clicks a control as the pointer would, pressing and releasing it over itself.
        ///
        /// \param Element The control clicked, which must be usable.
        /// \return `true` if it was clicked, `false` otherwise.
        Bool Trigger(Handle Element);

        /// \brief Closes the popup in front, else the modal pane in front, giving the keyboard back to what opened it.
        ///
        /// \return `true` if something closed, `false` otherwise.
        Bool Retreat();

        /// \brief Scrolls the innermost scrolling element holding the keyboard by a page, or to an end.
        ///
        /// \param Key The page key, home or end.
        /// \return `true` if an element holding the keyboard scrolls, `false` otherwise.
        Bool Page(ZyInput::Key Key);

    private:

        /// \brief Gets the arrow key that moves the keyboard one way.
        ///
        /// \param Way The way on screen the key moves toward.
        /// \return The arrow key pointing that way.
        static ZyInput::Key KeyOf(Heading Way);

        /// \brief Gets how far one box stands from another one way, and how far off that line, as the arrows weigh it.
        ///
        /// \param From   The box moved from.
        /// \param To     The box moved to.
        /// \param Way    The way on screen moved toward.
        /// \param Along  The receiver of the gap between the edges facing each other, in pixels.
        /// \param Across The receiver of how far apart their middles stand across that way.
        /// \return `true` if the middle of the box stands past the far edge of the other, `false` otherwise.
        static Bool Gauge(Rect From, Rect To, Heading Way, Ref<Real32> Along, Ref<Real32> Across);

        /// \brief Gets how far a view must scroll along one axis so a span of an element stands in it.
        ///
        /// \param Near     The near edge of the element on that axis.
        /// \param Far      The far edge of the element on that axis.
        /// \param ViewNear The near edge of the view on that axis.
        /// \param ViewFar  The far edge of the view on that axis.
        /// \return The distance to scroll, toward the far edge when positive, or nought when it already stands in view.
        static Real32 Nudge(Real32 Near, Real32 Far, Real32 ViewNear, Real32 ViewFar);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Tree>          mTree;
        Ref<Dispatcher>    mDispatcher;
        Ref<Pointer>       mPointer;
        Handle             mFocused;
        Handle             mRevealed;
        Sequence<Shortcut> mShortcuts;
    };
}