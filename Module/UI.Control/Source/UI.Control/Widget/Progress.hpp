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
#include "UI.Core/Handle.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the label a progress bar shows its share in, kept by handle as a layout may add children.
    struct Gauge final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The label showing the share, or one naming nothing for a bar made without one.
        Handle Readout;
    };

    /// \brief Represents a progress bar, a well filled up to its value that shows the share it stands at.
    class Progress final
    {
    public:

        /// \brief Creates a progress bar.
        ///
        /// \param Parent   The element to hold it.
        /// \param Value    The value it starts at.
        /// \param Minimum  The lowest value it shows.
        /// \param Maximum  The highest value it shows.
        /// \param Labelled The flag, `true` to show its share, as `40%`, over its middle, `false` to show none.
        /// \param Style    The name of the style it wears, its label wearing it followed by `.Label`.
        /// \return The progress bar, filled up to its value.
        static Element Create(Element Parent,
                              Real32  Value,
                              Real32  Minimum  = 0.0f,
                              Real32  Maximum  = 1.0f,
                              Bool    Labelled = true,
                              Text    Style    = "Progress");

        /// \brief Creates a progress bar from a node, at `Value` within `Minimum` and `Maximum`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the progress bar is read from.
        /// \return The progress bar, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Moves a progress bar's value, filling it and showing the share it stands at.
        ///
        /// \param Target The progress bar to move.
        /// \param Value  The value, held within the range.
        static void SetValue(Element Target, Real32 Value);

        /// \brief Gets a progress bar's value.
        ///
        /// \param Target The progress bar to read.
        /// \return The value it stands at, within its range.
        static Real32 GetValue(Element Target);
    };
}