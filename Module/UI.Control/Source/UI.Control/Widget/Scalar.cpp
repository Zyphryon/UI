// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Scalar.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Panel.hpp"
#include "Textbox.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scalar::Create(Element Parent, Real64 Value, UInt32 Decimals, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style, Direction::Row);
        Result.EditBox().Items = Align::Center;
        Widget::Govern(Result, Handler::Create<&Scalar::OnEvent>(), false);

        // The tag stands first, hidden until it is given a text.
        const Element Chip = Label::Create(Result, Text(), Widget::Part(Style, "Tag"));
        Chip.SetState(State::Hidden, true);

        const Element Field = Textbox::Create(Result, Text(), Text(), Widget::Part(Style, "Field"));
        Field.EditBox().Grow = 1.0f;
        Textbox::SetNumeric(Field, true);

        Ref<Quantity> Data = Result.Add<Quantity>();
        Data.Value    = Value;
        Data.Decimals = Min(Decimals, kMaxDecimals);
        Data.Field    = Field.GetHandle();

        Show(Result);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scalar::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent,
            Markup::GetNumber(Node, "Value"),
            static_cast<UInt32>(Max(Markup::GetNumber(Node, "Decimals"), 0.0f)),
            Markup::GetText(Node, "Style", "Scalar"));

        SetRange(Result, Markup::GetNumber(Node, "Minimum"), Markup::GetNumber(Node, "Maximum"));
        SetTag(Result, Markup::GetText(Node, "Tag"));

        if (const Text Style = Markup::GetText(Node, "TagStyle"); !Style.IsEmpty())
        {
            SetTagStyle(Result, Style);
        }

        // The range binds before the value, so the value shown first is held within the range it is given.
        Reader.Bind(Result, Node, "Minimum", Binder::Put<Real64, &Scalar::SetMinimum>());
        Reader.Bind(Result, Node, "Maximum", Binder::Put<Real64, &Scalar::SetMaximum>());
        Reader.Bind(Result, Node, "Tag", Binder::PutText<&Scalar::SetTag>());
        Reader.Bind(Result, Node, "Value", Binder::Put<Real64, &Scalar::SetValue>(),
                    Binder::Take<Real64, &Scalar::GetValue>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scalar::SetValue(Element Target, Real64 Value)
    {
        const Ptr<Quantity> Data = Target.Get<Quantity>();
        Data->Value = Hold(* Data, Value);

        // Text typed and not yet committed stays, and the value shows once the typing ends.
        if (!Target.GetTree().Get<Entry>(Data->Field)->Editing)
        {
            Show(Target);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real64 Scalar::GetValue(Element Target)
    {
        return Target.Get<Quantity>()->Value;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scalar::SetRange(Element Target, Real64 Minimum, Real64 Maximum)
    {
        const Ptr<Quantity> Data = Target.Get<Quantity>();
        Data->Minimum = Minimum;
        Data->Maximum = Maximum;
        SetValue(Target, Data->Value);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scalar::SetMinimum(Element Target, Real64 Minimum)
    {
        SetRange(Target, Minimum, Target.Get<Quantity>()->Maximum);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scalar::SetMaximum(Element Target, Real64 Maximum)
    {
        SetRange(Target, Target.Get<Quantity>()->Minimum, Maximum);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scalar::SetDecimals(Element Target, UInt32 Decimals)
    {
        Target.Get<Quantity>()->Decimals = Min(Decimals, kMaxDecimals);
        SetValue(Target, GetValue(Target));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scalar::SetTag(Element Target, Text Content)
    {
        const Element Chip = GetTag(Target);
        Label::SetText(Chip, Content);
        Chip.SetState(State::Hidden, Content.IsEmpty());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scalar::SetTagStyle(Element Target, Text Style)
    {
        GetTag(Target).SetStyle(Style);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scalar::GetTag(Element Target)
    {
        return Target.GetFirst();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scalar::GetField(Element Target)
    {
        return Element(Target.GetTree(), Target.Get<Quantity>()->Field);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Scalar::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const Ptr<Quantity> Data  = Owner.Get<Quantity>(Self);
        const Handle        Field = Data->Field;

        // A press on the tag types into the field as one on the field itself does.
        if (Message.Target == Self)
        {
            if (Message.Kind == Signal::kPress && Message.Button == ZyInput::Button::Left)
            {
                Owner.Focus(Field);
            }
            return false;
        }

        if (Message.Target != Field)
        {
            return false;
        }

        switch (Message.Kind)
        {
        case Signal::kFocus:
            Data->Before = Data->Value;
            Data->Fresh  = true;
            SelectAll(Owner, Field);
            return false;
        case Signal::kClick:
            // The press placed the caret, so the click that gave the field the keyboard selects the whole value again.
            if (Data->Fresh)
            {
                Data->Fresh = false;
                SelectAll(Owner, Field);
            }
            return false;
        case Signal::kBegin:
        case Signal::kCommit:
        case Signal::kEnd:
            // The text field's edges are the number field's own, so an undo step opens and closes around the typing.
            Data->Fresh = false;
            Owner.Dispatch(Event(Message.Kind, Self));
            return true;
        case Signal::kChange:
            Read(Owner, Self);
            return true;
        case Signal::kKey:
            // Enter has committed the text already, so the value shows again with its decimals.
            if (Message.Key == ZyInput::Key::Enter)
            {
                Show(Element(Owner, Self));
            }
            return false;
        case Signal::kBlur:
            Data->Fresh = false;
            Show(Element(Owner, Self));
            return false;
        default:
            return false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scalar::Read(Ref<Interface> Owner, Handle Self)
    {
        const Element            Target(Owner, Self);
        const ConstPtr<Quantity> Data    = Target.Get<Quantity>();
        const Text               Content = Textbox::GetText(Element(Owner, Data->Field));

        // The text the typing started from stands for the value held then, so escape puts back exactly what was there.
        Str Start;
        Start.AppendReal(Data->Before, Data->Decimals);

        Real64 Value = Data->Before;

        if (Content != Start)
        {
            // Only text read whole as a number moves the value, so a lone sign or point waits for its digits.
            UInt         Cursor = 0;
            const Real64 Number = StrExtractNumber<Real64>(Content, Cursor);

            if (Cursor == 0 || Cursor != Content.GetSize())
            {
                return;
            }

            const Real64 Scale = Pow10<Real64>(static_cast<SInt32>(Data->Decimals));
            Value = Hold(* Data, ::Round(Number * Scale) / Scale);
        }

        if (Value != Data->Value)
        {
            SetValue(Target, Value);
            Owner.Dispatch(Event(Signal::kChange, Self));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scalar::Show(Element Target)
    {
        const ConstPtr<Quantity> Data = Target.Get<Quantity>();

        Str Shown;
        Shown.AppendReal(Data->Value, Data->Decimals);
        Textbox::SetText(Element(Target.GetTree(), Data->Field), Shown);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scalar::SelectAll(Ref<Interface> Owner, Handle Field)
    {
        const Element Shown(Owner, Field);
        Textbox::Select(Shown, 0, static_cast<UInt32>(Textbox::GetText(Shown).GetSize()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real64 Scalar::Hold(ConstRef<Quantity> Data, Real64 Value)
    {
        return Data.Minimum < Data.Maximum ? Clamp(Value, Data.Minimum, Data.Maximum) : Value;
    }
}