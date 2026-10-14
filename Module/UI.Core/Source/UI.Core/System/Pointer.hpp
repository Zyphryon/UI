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
#include "Layout.hpp"
#include "UI.Core/Handle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    class Interface;

    /// \brief Represents the pointer over an interface, with what it rests on, presses and carries.
    class Pointer final
    {
    public:

        /// \brief Represents the code a module runs at the points of the pointer's work, each part optional.
        struct Hook final
        {
            /// \brief Defines the code run as a button goes down or comes up, returning `true` to take it.
            using OnButton  = Delegate<Bool(Ref<Interface> Owner, ZyInput::Button Button)>;

            /// \brief Defines the code run once a press found what it lands on.
            using OnPressed = Delegate<void(Ref<Interface> Owner, ZyInput::Button Button)>;

            /// \brief Defines the code run once a press was dispatched, told whether a handler took it.
            using OnOffered = Delegate<void(Ref<Interface> Owner, ZyInput::Button Button, Bool Stopped)>;

            /// \brief Defines the code run as the pointer moves, returning `true` to take the move.
            using OnMove    = Delegate<Bool(Ref<Interface> Owner)>;

            /// \brief Defines the code told of the pointer's work.
            using OnPointer = Delegate<void(Ref<Interface> Owner)>;

            /// \brief Defines the code run as a key goes down, returning `true` to take it.
            using OnKey     = Delegate<Bool(Ref<Interface> Owner, ZyInput::Key Key)>;

            /// \brief Defines the code run once the wheel's turn was offered to the controls, told whether one took it.
            using OnTurned  = Delegate<void(Ref<Interface> Owner, Bool Stopped)>;

            /// The code run first as a button goes down, which ends the press there by taking it.
            OnButton  Press;

            /// The code run once the press found what it lands on and closed the popups it missed.
            OnPressed Pressed;

            /// The code run once the press was dispatched.
            OnOffered Offered;

            /// The code run as a button comes up, which ends the release there by taking it.
            OnButton  Release;

            /// The code run as the pointer moves, before the control held hears it, ending the move by taking it.
            OnMove    Move;

            /// The code run as the wheel turns, once what the pointer rests on was found again.
            OnPointer Wheel;

            /// The code run once the wheel's turn was offered to the controls and scrolled what no control took it for.
            OnTurned  Turned;

            /// The code run at the end of every finding of what the pointer rests on.
            OnPointer Picked;

            /// The code run first as a key goes down, which keeps the focused control from hearing it by taking it.
            OnKey     Key;
        };

    public:

        /// \brief Constructs a pointer resting on nothing, over the elements of an interface.
        ///
        /// \param Owner      The interface whose elements it points at, which must outlive it.
        /// \param Layout     The layout, whose target the panes kept on screen stay inside.
        /// \param Dispatcher The dispatcher its events go through.
        Pointer(Ref<Interface> Owner, Ref<Layout> Layout, Ref<Dispatcher> Dispatcher);

        /// \brief Gets the element the pointer rests on.
        ///
        /// \return The element, or one naming nothing when the pointer passes through to the world.
        ZY_INLINE Handle GetOver() const
        {
            return mOver;
        }

        /// \brief Gets where the pointer is.
        ///
        /// \return The point, in units of the interface.
        ZY_INLINE Vector2 GetPosition() const
        {
            return mPointer;
        }

        /// \brief Gets the control the left button went down on and has not come up from yet.
        ///
        /// \return The control, or one naming nothing while the button is up or the press became a drag.
        ZY_INLINE Handle GetPressed() const
        {
            return mPressed[ZyEnum::Cast(ZyInput::Button::Left)];
        }

        /// \brief Gets the control a button went down on and has not come up from yet.
        ///
        /// \param Button The button, where any but the left one holds only a control taking every button.
        /// \return The control, or one naming nothing while the button is up or holds nothing.
        ZY_INLINE Handle GetPressed(ZyInput::Button Button) const
        {
            return mPressed[ZyEnum::Cast(Button)];
        }

        /// \brief Checks whether a button of the pointer is down, whatever took its press, as a drag.
        ///
        /// \return `true` while one is, `false` otherwise.
        ZY_INLINE Bool IsHeld() const
        {
            return mHeld.Any();
        }

        /// \brief Checks whether one button of the pointer is down, whatever took its press.
        ///
        /// \param Button The button asked about.
        /// \return `true` while it is, `false` otherwise.
        ZY_INLINE Bool IsHeld(ZyInput::Button Button) const
        {
            return mHeld.Test(ZyEnum::Cast(Button));
        }

        /// \brief Finds the point of the layout under the pointer at a new pixel ratio, keeping the pixel it stands on.
        ///
        /// \param Ratio The pixels of the target one unit of the interface covers.
        void Rescale(Real32 Ratio);

        /// \brief Moves the pointer.
        ///
        /// \param X      The pointer's horizontal position, in pixels.
        /// \param Y      The pointer's vertical position, in pixels.
        /// \param DeltaX The horizontal movement since the last event, in pixels.
        /// \param DeltaY The vertical movement since the last event, in pixels.
        /// \param Ratio  The pixels of the target one unit of the interface covers.
        /// \return `true` if the pointer rests on the interface, `false` otherwise.
        Bool OnMouseMove(Real32 X, Real32 Y, Real32 DeltaX, Real32 DeltaY, Real32 Ratio);

        /// \brief Presses a pointer button, where the left one may start a drag and the right one opens a context menu.
        ///
        /// \param Button The button going down.
        /// \return `true` if the press lands on the interface, `false` otherwise.
        Bool OnMouseDown(ZyInput::Button Button);

        /// \brief Releases a pointer button, clicking the control it went down on if still over it.
        ///
        /// \param Button The button coming up.
        /// \return `true` if the press had landed on the interface, `false` otherwise.
        Bool OnMouseUp(ZyInput::Button Button);

        /// \brief Lets every button go without a click, as when the window loses the keyboard.
        void Abandon();

        /// \brief Turns the wheel, scrolling the innermost scrolling element under the pointer.
        ///
        /// \param DeltaX The horizontal turn, in notches.
        /// \param DeltaY The vertical turn, in notches.
        /// \param Shift  The flag, `true` to turn a vertical turn sideways, as while shift is held.
        /// \return `true` if the pointer rests on the interface, `false` otherwise.
        Bool OnMouseScroll(Real32 DeltaX, Real32 DeltaY, Bool Shift);

        /// \brief Opens a popup at a point, over every pane of its band and kept on screen.
        ///
        /// \param Popup   The root of the popup.
        /// \param At      The point its top left corner opens at, in pixels of the target.
        /// \param Subject The element it was opened over, or one naming nothing.
        void Summon(Handle Popup, Vector2 At, Handle Subject = Handle());

        /// \brief Keeps the root of a pane on screen, moving it back inside the target whenever it overruns an edge.
        ///
        /// \param Root The root, which must be placed by its distance from the target's top left corner.
        /// \param Gap  The gap it keeps from the pointer, standing above it instead of off the bottom, or nought.
        void Confine(Handle Root, Real32 Gap = 0.0f);

        /// \brief Checks whether a popup was opened at a point, by \ref Summon, rather than by the control holding it.
        ///
        /// \param Root The root of the popup.
        /// \return `true` if it was, `false` otherwise.
        Bool IsSummoned(Handle Root) const;

        /// \brief Finds what the pointer rests on again, moving the hover from the controls it left to the new ones.
        void Repick();

        /// \brief Destroys every popup whose opener was destroyed, the submenus opened from it included.
        void Prune();

        /// \brief Moves the pointer's own clock on, which tells a double click from two clicks.
        ///
        /// \param Delta The time since the last update, in seconds.
        void Tick(Real32 Delta);

        /// \brief Moves every pane kept on screen back inside the target.
        ///
        /// \return `true` if one moved, so the panes must be laid out again, `false` otherwise.
        Bool Fence();

        /// \brief Adds the code a module runs at the points of the pointer's work.
        ///
        /// \param Entry The hook, whose parts left empty are passed over.
        void AddHook(Hook Entry);

        /// \brief Lets every hook take a key going down before the control holding the keyboard hears it.
        ///
        /// \param Key The key going down.
        /// \return `true` if a hook took it, `false` otherwise.
        Bool OnKey(ZyInput::Key Key);

        /// \brief Lets go of the control a button went down on without a click, as a press becoming a drag does.
        void Unpress();

        /// \brief Finds the nearest control holding an element, or the element itself.
        ///
        /// \param Element   The element the search starts from.
        /// \param Focusable The flag, `true` to find only one that takes the keyboard, `false` to find any.
        /// \return The control, or one naming nothing.
        Handle Nearest(Handle Element, Bool Focusable) const;

    private:

        /// The distance one notch of the wheel scrolls, in pixels.
        static constexpr Real32 kNotch     = 40.0f;

        /// The distance the pointer moves with a button held before a press on a draggable becomes a drag, in pixels.
        static constexpr Real32 kThreshold = 4.0f;

        /// The time within which a press must follow the last to count with it, in seconds.
        static constexpr Real32 kRepeat    = 0.4f;

        /// The number of buttons a pointer has, each held down or not.
        static constexpr UInt   kButtons   = ZyEnum::Count<ZyInput::Button>();

        /// \brief Represents the root of a pane kept on screen, and the gap it keeps from the pointer.
        struct Fenced final
        {
            /// The root of the pane kept on screen.
            Handle Root;

            /// The gap it keeps from the pointer, standing above it instead of running off the bottom, or nought.
            Real32 Gap = 0.0f;
        };

        /// \brief Represents the presses of one button in a row, which tell a double click from two clicks.
        struct Streak final
        {
            /// The control the last press of the button landed on.
            Handle  On;

            /// Where the last press landed, in units of the interface.
            Vector2 At;

            /// When the last press landed, on the pointer's own clock.
            Real32  Time  = 0.0f;

            /// How many presses came in a row.
            UInt32  Count = 0;
        };

        /// \brief Offers a moment of the pointer's work to every hook holding a part for it.
        ///
        /// \param Values The arguments handed to each part after the interface.
        /// \return `true` if a hook took it, `false` otherwise.
        template<auto Part, typename... Arguments>
        Bool Offer(Arguments... Values);

        /// \brief Tells every hook holding a part for a moment of the pointer's work of it.
        ///
        /// \param Values The arguments handed to each part after the interface.
        template<auto Part, typename... Arguments>
        void Notify(Arguments... Values);

        /// \brief Finds the element under a point, the front pane first and its last drawn element winning.
        ///
        /// \param Point The point, in pixels of the target.
        /// \return The element, or one naming nothing when the point passes through every pane or a modal holds it.
        Handle Pick(Vector2 Point);

        /// \brief Hides every open popup the pointer is not over, as a press outside closes a menu.
        void Dismiss();

        /// \brief Checks whether a control takes every button, so any button holds it as the left one does.
        ///
        /// \param Target The control pressed.
        /// \return `true` if it does, `false` otherwise.
        Bool TakesAny(Handle Target) const;

        /// \brief Checks whether a control asks to hear the pointer come over it, move and leave.
        ///
        /// \param Target The control the pointer came over, moved over or left.
        /// \return `true` if it is alive, standing in the tree and asking, `false` otherwise.
        Bool Hovers(Handle Target) const;

        /// \brief Tells a control asking for hover events that the pointer came over it, moved over it or left it.
        ///
        /// \param Kind   The signal, \ref Signal::kEnter, \ref Signal::kMove or \ref Signal::kLeave.
        /// \param Target The control told.
        void Tell(UInt64 Kind, Handle Target);

        /// \brief Tells the controls asking for hover events which ones the pointer left and which it came over.
        void Announce();

        /// \brief Scrolls the innermost scrolling element holding another that scrolls the way a wheel turned.
        ///
        /// \param Element The element the pointer rests on.
        /// \param Across  The sideways turn, in notches, toward the right.
        /// \param Down    The up and down turn, in notches, toward the top.
        void Wheel(Handle Element, Real32 Across, Real32 Down);

        /// \brief Takes hold of the thumb of the innermost scrolling element whose track stands under the pointer.
        ///
        /// \param Element The element the pointer rests on.
        /// \return `true` if a thumb was taken, which a press on its track first moves to the pointer.
        Bool Grip(Handle Element);

        /// \brief Scrolls the element whose thumb is held so the thumb follows the pointer.
        void Slide();

        /// \brief Gets where the room inside an element's padding starts, in pixels.
        ///
        /// \param Element The element.
        /// \return The top of its room.
        Real32 GetRoomTop(Handle Element) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Interface>          mOwner;
        Ref<Layout>             mLayout;
        Ref<Dispatcher>         mDispatcher;
        Vector2                 mCursor;
        Vector2                 mPointer;
        Handle                  mOver;
        Array<Handle, kButtons> mPressed;
        Handle                  mGrip;
        Real32                  mGrab;
        Bitset<kButtons>        mHeld;
        Sequence<Handle>        mHovered;
        Sequence<Handle>        mReached;
        Bool                    mBlocked;
        Sequence<Fenced>        mConfined;
        Sequence<Handle>        mSummoned;
        Real32                  mClock;
        Array<Streak, kButtons> mStreaks;
        Sequence<Hook>          mHooks;
    };
}