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
    /// \brief Represents the value a number field holds and the range and digits it is held to.
    struct Quantity final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The value the field holds, within its range when it has one.
        Real64 Value    = 0.0;

        /// The lowest value it takes, a range holding only while it stands below the highest.
        Real64 Minimum  = 0.0;

        /// The highest value it takes, a range holding only while it stands above the lowest.
        Real64 Maximum  = 0.0;

        /// The value held when its text field took the keyboard, which escape puts back exactly.
        Real64 Before   = 0.0;

        /// The digits shown after the point, where nought keeps the value whole.
        UInt32 Decimals = 0;

        /// The text field the value is shown and typed in.
        Handle Field;

        /// `true` from the moment the text field takes the keyboard until the first click on it, which selects all.
        Bool   Fresh    = false;
    };

    /// \brief Represents a number field, a value shown as text behind an optional tag, typed into to change it.
    class Scalar final
    {
    public:

        /// The most digits a value is shown with after the point.
        static constexpr UInt32 kMaxDecimals = 9;

    public:

        /// \brief Creates a number field, with no range and no tag.
        ///
        /// \param Parent   The element to hold it.
        /// \param Value    The value it starts at.
        /// \param Decimals The digits shown after the point, where nought keeps the value whole.
        /// \param Style    The name of the style it wears, its parts adding `.Tag` and `.Field`.
        /// \return The number field, reporting each typing's edges as \ref Signal::kBegin and \ref Signal::kEnd.
        static Element Create(Element Parent, Real64 Value = 0.0, UInt32 Decimals = 0, Text Style = "Scalar");

        /// \brief Creates a number field from a node, at `Value` within `Minimum` and `Maximum`, with `Decimals`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the number field is read from.
        /// \return The number field, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Changes a number field's value silently, shown once any typing waiting to be committed ends.
        ///
        /// \param Target The number field to change.
        /// \param Value  The value, held within the range when it has one.
        static void SetValue(Element Target, Real64 Value);

        /// \brief Gets a number field's value.
        ///
        /// \param Target The number field to read.
        /// \return The value it holds, within its range when it has one.
        static Real64 GetValue(Element Target);

        /// \brief Sets the range a number field's value is held in, holding the value within it without reporting it.
        ///
        /// \param Target  The number field whose range is set.
        /// \param Minimum The lowest value it takes.
        /// \param Maximum The highest value it takes, where one not above the lowest leaves the value unbounded.
        static void SetRange(Element Target, Real64 Minimum, Real64 Maximum);

        /// \brief Sets the lowest value a number field takes, as a bound range's low end does.
        ///
        /// \param Target  The number field whose range is set.
        /// \param Minimum The lowest value it takes.
        static void SetMinimum(Element Target, Real64 Minimum);

        /// \brief Sets the highest value a number field takes, as a bound range's high end does.
        ///
        /// \param Target  The number field whose range is set.
        /// \param Maximum The highest value it takes.
        static void SetMaximum(Element Target, Real64 Maximum);

        /// \brief Sets how many digits a number field shows after the point, which a value typed is rounded to.
        ///
        /// \param Target   The number field to change.
        /// \param Decimals The digits, where nought keeps the value whole, at most \ref kMaxDecimals.
        static void SetDecimals(Element Target, UInt32 Decimals);

        /// \brief Shows a tag before a number field's value, as the letter of the axis it edits.
        ///
        /// \param Target  The number field whose tag is set.
        /// \param Content The text of the tag, where an empty text hides it.
        static void SetTag(Element Target, Text Content);

        /// \brief Changes the style a number field's tag wears, as an axis's own colour.
        ///
        /// \param Target The number field whose tag is restyled.
        /// \param Style  The name of the style the tag wears, as `Axis.X`.
        static void SetTagStyle(Element Target, Text Style);

        /// \brief Gets the tag a number field shows before its value.
        ///
        /// \param Target The number field to read.
        /// \return The tag, a label hidden while it holds no text.
        static Element GetTag(Element Target);

        /// \brief Gets the text field a number field shows its value and takes typing in.
        ///
        /// \param Target The number field to read.
        /// \return The text field, taking only digits, minus signs and points.
        static Element GetField(Element Target);

    private:

        /// \brief Runs a number field, reading its text field's report as its value, shown once typing ends.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it, from itself or from its text field.
        /// \param Self    The number field hearing the event.
        /// \return `true` if the event stops at the number field, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Reads the text typed into a number field as its value, leaving it while the text reads as none.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The number field typed into.
        static void Read(Ref<Interface> Owner, Handle Self);

        /// \brief Writes a number field's value into its text field with its decimals.
        ///
        /// \param Target The number field to show.
        static void Show(Element Target);

        /// \brief Selects the whole text of a number field's text field, so what is typed replaces the value.
        ///
        /// \param Owner The interface it stands in.
        /// \param Field The text field to select in.
        static void SelectAll(Ref<Interface> Owner, Handle Field);

        /// \brief Gets a value held within a number field's range, or as it is while the field has none.
        ///
        /// \param Data  The number field's value and range.
        /// \param Value The value to hold.
        /// \return The value, within the range when there is one.
        static Real64 Hold(ConstRef<Quantity> Data, Real64 Value);
    };
}