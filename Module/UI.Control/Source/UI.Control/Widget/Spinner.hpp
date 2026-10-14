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
    /// \brief Represents how far one step moves a scalar, and how many decimals it shows its value with.
    struct Stepper final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The amount one step adds or takes away.
        Real32 Step     = 1.0f;

        /// The digits shown after the point.
        UInt32 Decimals = 0;
    };

    /// \brief Represents a scalar, a value between a button taking a step away and one adding a step.
    class Spinner final
    {
    public:

        /// \brief Creates a scalar.
        ///
        /// \param Parent   The element to hold it.
        /// \param Value    The value it starts at.
        /// \param Minimum  The lowest value it steps to.
        /// \param Maximum  The highest value it steps to.
        /// \param Step     The amount one step adds or takes away.
        /// \param Decimals The digits shown after the point.
        /// \param Style    The name of the style it wears, its parts adding `.Decrease`, `.Label` and `.Increase`.
        /// \return The scalar, reporting \ref Signal::kChange between \ref Signal::kBegin and \ref Signal::kEnd.
        static Element Create(Element Parent,
                              Real32  Value,
                              Real32  Minimum,
                              Real32  Maximum,
                              Real32  Step     = 1.0f,
                              UInt32  Decimals = 0,
                              Text    Style    = "Spinner");

        /// \brief Creates a scalar from a node, at `Value` within `Minimum` and `Maximum`, moving by `Step`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the scalar is read from.
        /// \return The scalar, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Moves a scalar's value and shows it, without reporting it.
        ///
        /// \param Target The scalar to move.
        /// \param Value  The value, held within the range.
        static void SetValue(Element Target, Real32 Value);

        /// \brief Gets a scalar's value.
        ///
        /// \param Target The scalar to read.
        /// \return The value it stands at, within its range.
        static Real32 GetValue(Element Target);

        /// \brief Sets the lowest value a scalar steps to, holding its value within the range without reporting it.
        ///
        /// \param Target  The scalar whose range is set.
        /// \param Minimum The lowest value it steps to.
        static void SetMinimum(Element Target, Real32 Minimum);

        /// \brief Sets the highest value a scalar steps to, holding its value within the range without reporting it.
        ///
        /// \param Target  The scalar whose range is set.
        /// \param Maximum The highest value it steps to.
        static void SetMaximum(Element Target, Real32 Maximum);

    private:

        /// The most digits a value is shown with after the point, beyond which the digits overflow.
        static constexpr UInt32 kMaxDecimals = 6;

        /// \brief Runs a scalar, stepping its value on a press on a button, a turn of the wheel or an up or down key.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it.
        /// \param Self    The scalar hearing the event.
        /// \return `true` if the event asked it for a step, even one an end held back, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Moves a scalar by a number of steps, reporting the move when its value changed.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Self   The scalar to move.
        /// \param Amount The number of steps, below nought to take them away.
        static void Move(Ref<Interface> Owner, Handle Self, Real32 Amount);

        /// \brief Moves a scalar by steps as an edit of its own, between \ref Signal::kBegin and \ref Signal::kEnd.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Self   The scalar to move.
        /// \param Amount The number of steps, below nought to take them away.
        static void Nudge(Ref<Interface> Owner, Handle Self, Real32 Amount);
    };
}