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

#include "UI.Core/Style/Appearance.hpp"
#include "UI.Core/Style/Skin.hpp"
#include "UI.Core/Types.hpp"
#include <Zyphryon.Math/Motion/Easing.hpp>
#include <Zyphryon.Math/Motion/Playback.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents how an element moves, with the values it shows, the transition easing them and its animations.
    struct Motion final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty       kMarks   = Dirty::None;

        /// The values a look holds.
        static constexpr Skin::Field kShading =
              Skin::Field::Background
            | Skin::Field::Border
            | Skin::Field::BorderWidth
            | Skin::Field::Radius
            | Skin::Field::Opacity;

        /// The values an effect holds.
        static constexpr Skin::Field kEffects =
              Skin::Field::Gradient
            | Skin::Field::Shadow
            | Skin::Field::Glow
            | Skin::Field::InnerGlow
            | Skin::Field::Scale
            | Skin::Field::Rotate
            | Skin::Field::Translate;

        /// The values that move, every other one changing at once since it lays the element out again.
        static constexpr Skin::Field kMoving  =
              kShading
            | kEffects
            | Skin::Field::Tint
            | Skin::Field::Stroke
            | Skin::Field::StrokeWidth
            | Skin::Field::Fill
            | Skin::Field::Thumb
            | Skin::Field::ImageTint;

        /// The values a backdrop holds that change at once, only its tint moving.
        static constexpr Skin::Field kImaging =
              Skin::Field::Image
            | Skin::Field::Region
            | Skin::Field::Slice;

        /// The values that make an element hold a motion at all.
        static constexpr Skin::Field kDriven  =
              Skin::Field::Transition
            | Skin::Field::Animation
            | Skin::Field::Enter
            | Skin::Field::Exit;

        /// The values the element shows, its own with every animation played over them.
        Appearance  Shown;

        /// The values of its own, as the transition leaves them, before any animation is played over them.
        Appearance  Own;

        /// The values the running transition started from.
        Appearance  From;

        /// The values its skins give, which the transition ends at.
        Appearance  Goal;

        /// The values a skin or an animation ever moved, so their facets are kept.
        Skin::Field Touched  = Skin::Field::None;

        /// How far the running transition has run over its duration, its delay waited out as the time before nought.
        Playback    Clock;

        /// How the running transition eases along the way.
        Easing      Curve    = Easing::Linear;

        /// `true` while a transition runs.
        Bool        Shifting = false;

        /// The values the running transition moves, those where it started apart from where it ends.
        Skin::Field Moving   = Skin::Field::None;

        /// The identifier of the animation played over and over, or zero for none.
        UInt64      Loop     = 0;

        /// How far that animation has played, over every run it plays.
        Playback    LoopClock;

        /// The identifier of the animation played once as the element appears or leaves, or zero for none.
        UInt64      Once     = 0;

        /// How far that animation has played, over its single run.
        Playback    OnceClock;

        /// `true` while the element plays its way out, hidden to the pointer but laid out and drawn until it ends.
        Bool        Leaving  = false;

        /// `true` when the element itself was hidden the last time it was dressed.
        Bool        Hidden   = false;

        /// `true` while the element is in the interface's list of what moves.
        Bool        Listed   = false;
    };
}