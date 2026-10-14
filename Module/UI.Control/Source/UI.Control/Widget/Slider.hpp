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
    /// \brief Represents the value a slider, a progress bar or a scalar holds, and the range it moves in.
    struct Level final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::Paint;

        /// The value the widget stands at, kept within the range by whoever sets it.
        Real32 Value   = 0.0f;

        /// The lowest value it moves to.
        Real32 Minimum = 0.0f;

        /// The highest value it moves to.
        Real32 Maximum = 1.0f;

        /// \brief Gets the share of the range the value stands at.
        ///
        /// \return The share, over nought through one.
        ZY_INLINE Real32 GetShare() const
        {
            return InverseLerp(Minimum, Maximum, Value);
        }
    };

    /// \brief Represents a slider, a track filled up to its value with a thumb standing on it.
    class Slider final
    {
    public:

        /// \brief Creates a slider.
        ///
        /// \param Parent  The element to hold it.
        /// \param Value   The value it starts at.
        /// \param Minimum The lowest value it moves to.
        /// \param Maximum The highest value it moves to.
        /// \param Style   The name of the style it wears, its thumb wearing it followed by `.Thumb`.
        /// \return The slider, reporting \ref Signal::kChange between \ref Signal::kBegin and \ref Signal::kEnd.
        static Element Create(Element Parent,
                              Real32  Value,
                              Real32  Minimum = 0.0f,
                              Real32  Maximum = 1.0f,
                              Text    Style   = "Slider");

        /// \brief Creates a slider from a node, at `Value` within `Minimum` and `Maximum`, `Value` binding both ways.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the slider is read from.
        /// \return The slider, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Moves a slider's value, filling its track and placing its thumb, without reporting it.
        ///
        /// \param Target The slider to move.
        /// \param Value  The value, held within the range.
        static void SetValue(Element Target, Real32 Value);

        /// \brief Gets a slider's value.
        ///
        /// \param Target The slider to read.
        /// \return The value it stands at, within its range.
        static Real32 GetValue(Element Target);

        /// \brief Sets the lowest value a slider moves to, holding its value within the range without reporting it.
        ///
        /// \param Target  The slider whose range is set.
        /// \param Minimum The lowest value it moves to.
        static void SetMinimum(Element Target, Real32 Minimum);

        /// \brief Sets the highest value a slider moves to, holding its value within the range without reporting it.
        ///
        /// \param Target  The slider whose range is set.
        /// \param Maximum The highest value it moves to.
        static void SetMaximum(Element Target, Real32 Maximum);

    private:

        /// The share of its range an arrow key moves a slider by.
        static constexpr Real32 kStep = 0.05f;

        /// \brief Runs a slider, where a press or drag moves its value under the pointer, as do arrows, home and end.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it.
        /// \param Self    The slider hearing the event.
        /// \return `true` if the event was a left press, drag or release, or a key it keeps, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Moves a slider's value by a key, arrows by a twentieth of its range, home and end to either end.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The slider to move.
        /// \param Key   The key pressed while it holds the keyboard.
        /// \return `true` if it keeps the key, enter and space included, `false` otherwise.
        static Bool OnKey(Ref<Interface> Owner, Handle Self, ZyInput::Key Key);

        /// \brief Moves a slider's value as the pointer or a key asks, reporting \ref Signal::kChange when it changed.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The slider to move.
        /// \param Value The value asked for, held within the range.
        static void Move(Ref<Interface> Owner, Handle Self, Real32 Value);

        /// \brief Moves a slider's value by a key as one edit, between \ref Signal::kBegin and \ref Signal::kEnd.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The slider to move.
        /// \param Value The value asked for, held within the range.
        /// \return `true` always, the key having been the slider's.
        static Bool Nudge(Ref<Interface> Owner, Handle Self, Real32 Value);
    };
}