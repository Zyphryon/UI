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

#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the effects an element is drawn with besides its look, as a gradient, shadows and a transform.
    struct Effect final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::Paint;

        /// The gradient the box is filled with over its background, which shows wherever the gradient is not opaque.
        Gradient Fill;

        /// The shadow cast behind the box.
        Halo     Shadow;

        /// The glow around the box.
        Halo     Glow;

        /// The glow just inside the box's edge.
        Halo     InnerGlow;

        /// How much larger the element and all it holds are drawn on each axis, about its middle.
        Vector2  Scale  = Vector2(1.0f, 1.0f);

        /// How far the element and all it holds are drawn turned, clockwise, in degrees.
        Real32   Rotate = 0.0f;

        /// How far the element and all it holds are drawn moved, in pixels.
        Vector2  Translate;

        /// \brief Checks whether the transform moves anything.
        ///
        /// \return `true` if it scales, turns or moves the element, `false` otherwise.
        ZY_INLINE Bool IsTransformed() const
        {
            return !Scale.IsAlmostEqual(1.0f, 1.0f) || !IsAlmostZero(Rotate) || !Translate.IsAlmostZero();
        }
    };
}