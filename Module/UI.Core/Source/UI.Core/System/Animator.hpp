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

#include "UI.Core/Facet/Style/Motion.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Pane.hpp"
#include "UI.Core/Style/Animation.hpp"
#include "UI.Core/Style/Appearance.hpp"
#include "UI.Core/Style/Skin.hpp"
#include "UI.Core/Style/Theme.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents what moves the values of the elements of a tree, through their transitions and animations.
    class Animator final
    {
    public:

        /// \brief Defines the code reading the values that move from the facet of an element into an appearance.
        using Read  = Delegate<void(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output)>;

        /// \brief Defines the code writing an appearance onto the facet of an element.
        using Write = Delegate<void(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field Given)>;

        /// \brief Defines the code giving the facet of an element what a skin sets at once.
        using Fit   = Delegate<Bool(Ref<Tree> Owner, Handle Element, ConstRef<Skin> Values)>;

        /// \brief Represents how one kind of facet wears a skin, with the values that move and those set at once.
        struct Applier final
        {
            /// The code reading the values that move, or empty for none.
            Read  Capture;

            /// The code writing them, or empty for none.
            Write Apply;

            /// The code setting what a skin sets at once, or empty for none.
            Fit   Fitting;
        };

    public:

        /// \brief Constructs an animator moving nothing, over the elements of a tree.
        ///
        /// \param Owner The tree whose elements it moves, which must outlive it.
        Animator(Ref<Tree> Owner);

        /// \brief Sets the theme whose animations the elements play.
        ///
        /// \param Source The theme, or `nullptr` to play none.
        void SetTheme(ConstPtr<Theme> Source);

        /// \brief Starts, retargets or stops what an element moves through after a skin was worked out for it.
        ///
        /// \param Target  The pane holding the element.
        /// \param Row     The row of the element in that pane.
        /// \param Values  The skin worked out for the element, which its values move toward.
        /// \param State   The motion of the element, which keeps its clocks and the values it shows.
        /// \param Dressed The flag, `true` to move from the skin the element wore, `false` to snap to the new one.
        void Stir(Ref<Pane> Target, UInt32 Row, ConstRef<Skin> Values, Ref<Motion> State, Bool Dressed);

        /// \brief Moves every running transition and animation on, dropping the ones that ended.
        ///
        /// \param Delta The time since the last update, in seconds.
        void Tick(Real32 Delta);

        /// \brief Reads the values that move from the facets of an element.
        ///
        /// \param Slot The slot of the element whose facets are read.
        /// \return The values, at each facet's default where the element holds none.
        Appearance Capture(UInt32 Slot) const;

        /// \brief Writes the values that move into the facets of an element.
        ///
        /// \param Element The element whose facets are written.
        /// \param Values  The values written into them.
        /// \param Given   The values a skin or an animation gives, whose look or effect is added when missing.
        void Apply(Handle Element, ConstRef<Appearance> Values, Skin::Field Given);

        /// \brief Gives the facets of an element what a skin sets at once, as a typeface or an image.
        ///
        /// \param Element The element whose facets take the values.
        /// \param Values  The skin giving the values.
        /// \return `true` if a value that sizes the element changed, `false` otherwise.
        Bool Tailor(Handle Element, ConstRef<Skin> Values);

        /// \brief Adds how a kind of facet wears a skin, after the interface's own and those added before.
        ///
        /// \param Entry The applier reading and writing that kind of facet.
        void AddApplier(Applier Entry);

        /// \brief Checks whether a row is laid out and drawn, while not hidden or still playing its way out.
        ///
        /// \param Target The pane holding the row.
        /// \param Row    The row of the element in that pane.
        /// \return `true` if it is, `false` otherwise.
        Bool IsDrawn(ConstRef<Pane> Target, UInt32 Row) const;

    private:

        /// \brief Moves one element's transition and animations on to the animator's clock, showing where they stand.
        ///
        /// \param Element The element being moved.
        /// \param State   The motion of the element.
        /// \return `true` if anything of it still runs, `false` otherwise.
        Bool Advance(Handle Element, Ref<Motion> State);

        /// \brief Works out the values an element shows, its own with its animations played over them.
        ///
        /// \param State The motion of the element, whose shown values are written.
        void Compose(Ref<Motion> State);

        /// \brief Plays an animation over some values at a share of its run.
        ///
        /// \param Clip    The animation played.
        /// \param Share   The share of the run, over nought through one.
        /// \param Beneath The values beneath it, which stand at either end and wherever no key gives a value.
        /// \param Output  The receiver of the values, written only where a key gives one.
        /// \param Touched The receiver of the values any of its keys give, added to the ones it held.
        void Overlay(ConstRef<Animation>  Clip,
                     Real32               Share,
                     ConstRef<Appearance> Beneath,
                     Ref<Appearance>      Output,
                     Ref<Skin::Field>     Touched);

        /// \brief Plays one value of an animation, between the keys around a share of its run that give it.
        ///
        /// \param Clip    The animation, whose keys were lifted over the values beneath it.
        /// \param Share   The share of the run, over nought through one.
        /// \param Bit     The single value played.
        /// \param Beneath The values beneath the animation, which stand in past its first or last key.
        /// \param Output  The receiver of the value.
        void Tween(ConstRef<Animation>  Clip,
                   Real32               Share,
                   Skin::Field          Bit,
                   ConstRef<Appearance> Beneath,
                   Ref<Appearance>      Output) const;

        /// \brief Finds an animation of the theme.
        ///
        /// \param Name The identifier of the animation, or zero for none.
        /// \return The animation, or `nullptr` when there is no theme, no name or no such animation.
        ConstPtr<Animation> Seek(UInt64 Name) const;

        /// \brief Runs one of an element's clocks on from where it stands, from the animator's clock as it is now.
        ///
        /// \param Clock The clock, which runs on whether it was held, ended or never started.
        void Resume(Ref<Playback> Clock) const;

    public:

        /// \brief Gives some values that move every one of them a skin gives.
        ///
        /// \param Output The receiver of the values the skin gives, the rest kept.
        /// \param Values The skin giving the values.
        static void Lay(Ref<Appearance> Output, ConstRef<Skin> Values);

    private:

        /// \brief Mixes two colours channel by channel, keeping the hue of the visible end where the other is clear.
        ///
        /// \param From  The colour at nought.
        /// \param To    The colour at one.
        /// \param Share The share of the way from one to the other.
        /// \return The mixed colour.
        static Color Mix(Color From, Color To, Real32 Share);

        /// \brief Mixes two halos, each of their values on its own.
        ///
        /// \param From  The halo at nought.
        /// \param To    The halo at one.
        /// \param Share The share of the way from one to the other.
        /// \return The mixed halo.
        static Halo Mix(ConstRef<Halo> From, ConstRef<Halo> To, Real32 Share);

        /// \brief Mixes two gradients, their colours and their direction.
        ///
        /// \param From  The gradient at nought.
        /// \param To    The gradient at one.
        /// \param Share The share of the way from one to the other.
        /// \return The mixed gradient.
        static Gradient Mix(ConstRef<Gradient> From, ConstRef<Gradient> To, Real32 Share);

        /// \brief Mixes some of the values that move from one set toward another.
        ///
        /// \param Output The receiver of the mixed values, written only where the mask names one.
        /// \param From   The values at nought.
        /// \param To     The values at one.
        /// \param Share  The share of the way from one to the other.
        /// \param Mask   The values to mix.
        static void Blend(Ref<Appearance>      Output,
                          ConstRef<Appearance> From,
                          ConstRef<Appearance> To,
                          Real32               Share,
                          Skin::Field          Mask);

        /// \brief Checks whether two values differ in any bit, which a transition between them must ease.
        ///
        /// \param First  The one value.
        /// \param Second The other value.
        /// \return `true` if any bit differs, `false` otherwise.
        template<typename Type>
        static Bool Differs(ConstRef<Type> First, ConstRef<Type> Second);

        /// \brief Gets the values two sets hold apart, which a transition from one to the other must ease.
        ///
        /// \param From The values at nought.
        /// \param To   The values at one.
        /// \return The values that differ.
        static Skin::Field Apart(ConstRef<Appearance> From, ConstRef<Appearance> To);

        /// \brief Gets a facet of an element, adding it when the element holds none and one is needed.
        ///
        /// \param Owner   The tree the element stands in.
        /// \param Element The element whose facet is wanted.
        /// \param Needed  The flag, `true` to add the facet when the element holds none.
        /// \return The facet, or `nullptr` when the element holds none and none is needed.
        template<typename Type>
        static Ptr<Type> Obtain(Ref<Tree> Owner, Handle Element, Bool Needed);

    private:

        /// \brief Reads the values that move from a look, as the interface's own applier.
        ///
        /// \param Owner  The tree the element stands in.
        /// \param Slot   The slot of the element whose look is read.
        /// \param Output The receiver of the values it holds.
        static void ReadLook(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output);

        /// \brief Writes the values that move onto a look, as the interface's own applier.
        ///
        /// \param Owner   The tree the element stands in.
        /// \param Element The element whose look is written, which is given one when the skin shades it.
        /// \param Values  The values written.
        /// \param Given   The values a skin or an animation gives.
        static void WriteLook(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field Given);

        /// \brief Reads the values that move from text, as the interface's own applier.
        ///
        /// \param Owner  The tree the element stands in.
        /// \param Slot   The slot of the element whose caption is read.
        /// \param Output The receiver of the values it holds.
        static void ReadCaption(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output);

        /// \brief Writes the values that move onto text, as the interface's own applier.
        ///
        /// \param Owner   The tree the element stands in.
        /// \param Element The element whose caption is written, when it holds one.
        /// \param Values  The values written.
        /// \param Given   The values a skin or an animation gives, which never add a caption.
        static void WriteCaption(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field Given);

        /// \brief Reads the values that move from a bar, as the interface's own applier.
        ///
        /// \param Owner  The tree the element stands in.
        /// \param Slot   The slot of the element whose bar is read.
        /// \param Output The receiver of the values it holds.
        static void ReadBar(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output);

        /// \brief Writes the values that move onto a bar, as the interface's own applier.
        ///
        /// \param Owner   The tree the element stands in.
        /// \param Element The element whose bar is written, when it holds one.
        /// \param Values  The values written.
        /// \param Given   The values a skin or an animation gives, which never add a bar.
        static void WriteBar(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field Given);

        /// \brief Reads the values that move from a scrolling element, as the interface's own applier.
        ///
        /// \param Owner  The tree the element stands in.
        /// \param Slot   The slot of the element whose scroll is read.
        /// \param Output The receiver of the values it holds.
        static void ReadScroll(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output);

        /// \brief Writes the values that move onto a scrolling element, as the interface's own applier.
        ///
        /// \param Owner   The tree the element stands in.
        /// \param Element The element whose scroll is written, when it holds one.
        /// \param Values  The values written.
        /// \param Given   The values a skin or an animation gives, which never add a scroll.
        static void WriteScroll(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field Given);

        /// \brief Reads the values that move from effects, as the interface's own applier.
        ///
        /// \param Owner  The tree the element stands in.
        /// \param Slot   The slot of the element whose effects are read.
        /// \param Output The receiver of the values it holds.
        static void ReadEffect(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output);

        /// \brief Writes the values that move onto effects, as the interface's own applier.
        ///
        /// \param Owner   The tree the element stands in.
        /// \param Element The element whose effects are written, which is given them when the skin names one.
        /// \param Values  The values written.
        /// \param Given   The values a skin or an animation gives.
        static void WriteEffect(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field Given);

        /// \brief Reads the values that move from a skin image, as the interface's own applier.
        ///
        /// \param Owner  The tree the element stands in.
        /// \param Slot   The slot of the element whose skin image is read.
        /// \param Output The receiver of the values it holds.
        static void ReadBackdrop(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output);

        /// \brief Writes the values that move onto a skin image, as the interface's own applier.
        ///
        /// \param Owner   The tree the element stands in.
        /// \param Element The element whose skin image is written, which is given one when the skin tints it.
        /// \param Values  The values written.
        /// \param Given   The values a skin or an animation gives.
        static void WriteBackdrop(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field Given);

        /// \brief Gives text its size, typeface and alignment, as the interface's own applier.
        ///
        /// \param Owner   The tree the element stands in.
        /// \param Element The element whose caption takes the values, when it holds one.
        /// \param Values  The skin giving the values.
        /// \return `true` if its size or typeface changed, `false` otherwise.
        static Bool FitCaption(Ref<Tree> Owner, Handle Element, ConstRef<Skin> Values);

        /// \brief Gives a skin image its image, region and slice, as the interface's own applier.
        ///
        /// \param Owner   The tree the element stands in.
        /// \param Element The element whose skin image takes the values, which is given one when the skin names any.
        /// \param Values  The skin giving the values.
        /// \return `false` always, since an image is only drawn and never measured.
        static Bool FitBackdrop(Ref<Tree> Owner, Handle Element, ConstRef<Skin> Values);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Tree>            mTree;
        ConstPtr<Theme>      mTheme;
        Real64               mClock;
        Sequence<Handle>     mMoving;
        Sequence<Appearance> mLifted;
        Sequence<Applier>    mAppliers;
    };
}