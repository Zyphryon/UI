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

#include "Textbox.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Panel.hpp"
#include "Scrollbar.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Layout/Virtual.hpp"
#include "UI.Core/Facet/Visual/Caption.hpp"
#include "UI.Core/Facet/Visual/Look.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Text/Block.hpp"
#include "UI.Core/Text/Ruler.hpp"
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

    Element Textbox::Create(Element Parent, Text Content, Text Hint, Text Style)
    {
        const Element Result = Parent.Create();

        Ref<Box> Shape = Result.EditBox();
        Shape.Flow  = Direction::Row;
        Shape.Items = Align::Center;

        Widget::Govern(Result, Handler::Create<&Textbox::OnEvent>(), true);
        Result.SetStyle(Style);

        // A field holding the keyboard takes the keys an application's shortcuts would, copying when read-only.
        Result.Get<Control>()->Typing = true;

        Ref<Entry> Data = Result.Add<Entry>();
        Data.Caret  = static_cast<UInt32>(Content.GetSize());
        Data.Anchor = Data.Caret;

        // A long line scrolls sideways, thumb unseen, so the caret stays in view; the clip cuts it to the field's box.
        Ref<Scroll> Roll = Result.Add<Scroll>();
        Roll.Horizontal = true;
        Roll.Gutter     = 0.0f;
        Roll.Thumb      = Color::Transparent();

        Result.Add<Look>().Clip = true;

        // The caret and the selection stand where the text is, placed once the field has been laid out.
        Result.Add<Virtual>().Realize = Realizer::Create<&Textbox::OnRealize>();

        // The parts stand in a fixed order: text, caret, hint, then one selection band per line, made as needed.
        const Element Shown = Label::Create(Result, Content, Widget::Part(Style, "Text"));
        const Element Caret = Panel::Create(Result, Widget::Part(Style, "Caret"));
        const Element Muted = Label::Create(Result, Hint, Widget::Part(Style, "Hint"));
        const Element Band  = Panel::Create(Result, Widget::Part(Style, "Selection"));

        Shown.Edit<Caption>()->Breaks = false;
        Muted.Edit<Caption>()->Breaks = false;

        Caret.EditBox().Placement = Position::Absolute;
        Band.EditBox().Placement  = Position::Absolute;
        Band.SetState(State::Hidden, true);

        Refresh(Result);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Textbox::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent,
            Markup::GetText(Node, "Text"),
            Markup::GetText(Node, "Hint"),
            Markup::GetText(Node, "Style", "Textbox"));

        const Ptr<Entry> Data = Result.Get<Entry>();
        Data->Limit    = static_cast<UInt32>(Max(Markup::GetNumber(Node, "Limit"), 0.0f));
        Data->ReadOnly = Markup::GetBool(Node, "ReadOnly");
        Data->Numeric  = Markup::GetBool(Node, "Numeric");
        SetMultiline(Result, Markup::GetBool(Node, "Multiline"));

        Reader.Bind(Result, Node, "Text", Binder::PutText<&Textbox::SetText>(),
                    Binder::Take<Text, &Textbox::GetText>());
        Reader.Bind(Result, Node, "Hint", Binder::PutText<&Textbox::SetHint>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Textbox::SetText(Element Target, Text Content)
    {
        if (GetText(Target) == Content)
        {
            return;
        }

        Label::SetText(Target.GetFirst(), Content);

        if (const Ptr<Entry> Data = Target.Get<Entry>())
        {
            Data->Caret  = static_cast<UInt32>(Content.GetSize());
            Data->Anchor = Data->Caret;
            Data->Goal   = -1.0f;

            // Text set with no edit under way is what escape returns to and what a commit is weighed against.
            if (!Data->Editing && Data->Original != Content)
            {
                Data->Original = Content;
            }
        }
        Refresh(Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text Textbox::GetText(Element Target)
    {
        const ConstPtr<Caption> Source = Target.GetFirst().Get<Caption>();
        return Source ? Text(Source->Content) : Text();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Textbox::SetHint(Element Target, Text Content)
    {
        // The hint is the third part, after the text and the caret.
        Label::SetText(Target.GetFirst().GetNext().GetNext(), Content);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Textbox::SetMultiline(Element Target, Bool Enabled)
    {
        const Ptr<Entry> Data = Target.Get<Entry>();

        if (!Data || Data->Multiline == Enabled)
        {
            return;
        }
        Data->Multiline = Enabled;

        // Several lines stand in a column across the field and scroll up and down, a thumb in a gutter showing where.
        Ref<Box> Shape = Target.EditBox();
        Shape.Flow  = Enabled ? Direction::Column : Direction::Row;
        Shape.Items = Enabled ? Align::Stretch : Align::Center;

        if (Enabled && Shape.Height.IsAuto())
        {
            Shape.Height = Length::Pixels(kHeight);
        }

        const Ptr<Scroll> Roll = Target.Edit<Scroll>();
        Roll->Horizontal = !Enabled;
        Roll->Gutter     = Enabled ? 8.0f : 0.0f;
        Roll->Thumb      = Enabled ? Color::White(0.3f) : Color::Transparent();
        Roll->Offset     = 0.0f;
        Roll->OffsetX    = 0.0f;

        Target.GetFirst().Edit<Caption>()->Breaks                     = Enabled;
        Target.GetFirst().GetNext().GetNext().Edit<Caption>()->Breaks = Enabled;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Textbox::SetReadOnly(Element Target, Bool Enabled)
    {
        if (const Ptr<Entry> Data = Target.Get<Entry>())
        {
            Data->ReadOnly = Enabled;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Textbox::SetNumeric(Element Target, Bool Enabled)
    {
        if (const Ptr<Entry> Data = Target.Get<Entry>())
        {
            Data->Numeric = Enabled;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Textbox::Select(Element Target, UInt32 Anchor, UInt32 Caret)
    {
        const Ptr<Entry> Data    = Target.Get<Entry>();
        const Text       Content = GetText(Target);

        if (!Data)
        {
            return;
        }

        Data->Anchor = static_cast<UInt32>(Unicode::Snap(Content, Anchor));
        Data->Caret  = static_cast<UInt32>(Unicode::Snap(Content, Caret));
        Data->Goal   = -1.0f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Textbox::GetCaret(Element Target)
    {
        const ConstPtr<Entry> Data = Target.Get<Entry>();
        return Data ? Min<UInt32>(Data->Caret, static_cast<UInt32>(GetText(Target).GetSize())) : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text Textbox::GetSelected(Element Target)
    {
        const ConstPtr<Entry> Data    = Target.Get<Entry>();
        const Text            Content = GetText(Target);

        if (!Data)
        {
            return Text();
        }

        const AnyRange<UInt> Selection = GetSelection(* Data, Content.GetSize());
        return Content.Slice(Selection.GetMinimum(), Selection.GetLength());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Textbox::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        if (Message.Target != Self)
        {
            return false;
        }

        const Element    Target(Owner, Self);
        const Ptr<Entry> Data   = Target.Get<Entry>();
        Bool             Edited = false;

        switch (Message.Kind)
        {
        case Signal::kType:
            Edited = Insert(Target, Message.Content);
            break;
        case Signal::kKey:
            // Escape keeps the key only when it put something back, so a popup holding an untouched field closes.
            if (Message.Key == ZyInput::Key::Escape)
            {
                return Revert(Owner, Target);
            }

            // Enter commits a line and bubbles on, so a dialog holding the field still hears it.
            if (Message.Key == ZyInput::Key::Enter && !Data->Multiline)
            {
                Commit(Owner, Target, true);
                return false;
            }

            // The other keys bubble on, so whatever holds the field hears tab.
            if (!OnKey(Owner, Target, Message.Key, Edited))
            {
                return false;
            }
            break;
        case Signal::kPress:
            // Presses bubble on, so a window holding the field hears them.
            if (Message.Button == ZyInput::Button::Left)
            {
                OnPress(Owner, Target, Message);
            }
            return false;
        case Signal::kDrag:
            if (Data->Selecting)
            {
                Move(Target, Locate(Owner, Target, Message.Position), true);
            }
            return false;
        case Signal::kRelease:
            Data->Selecting = false;
            return false;
        case Signal::kFocus:
            Data->Original = GetText(Target);
            Data->Editing  = false;
            return false;
        case Signal::kBlur:
            Data->Selecting = false;
            Commit(Owner, Target, false);
            return false;
        default:
            return false;
        }

        if (Edited)
        {
            // The first change since the last commit opens the edit, before the change, so an undo step can begin.
            if (!Data->Editing)
            {
                Data->Editing = true;
                Owner.Dispatch(Event(Signal::kBegin, Self));
            }
            Owner.Dispatch(Event(Signal::kChange, Self));
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Textbox::OnRealize(Ref<Interface> Owner, Handle Self)
    {
        const Element         Target(Owner, Self);
        const ConstPtr<Entry> Data  = Target.Get<Entry>();
        const Element         Caret = Target.GetFirst().GetNext();
        Element               Band  = Caret.GetNext().GetNext();
        Bool                  Moved = false;

        // Kept from call to call, since the field holding the keyboard is surveyed every frame.
        thread_local Block Shape;

        // Only the field holding the keyboard shows where its caret and selection stand.
        if (!Data || !Target.HasState(State::Focus) || !Survey(Owner, Target, Shape) || !Shape.Source)
        {
            for (; Band.IsAlive(); Band = Band.GetNext())
            {
                Moved = Show(Band, false) || Moved;
            }
            return Moved;
        }

        const Text           Content   = GetText(Target);
        const Rect           Frame     = Target.GetFrame();
        const Vector2        Corner    = Frame.GetPosition();
        const AnyRange<UInt> Selection = GetSelection(* Data, Content.GetSize());
        const Vector2        At        = Ruler::Place(Shape, Content, Min<UInt>(Data->Caret, Content.GetSize()));
        const Real32         Wide      = Max(Caret.GetFrame().GetWidth(), 1.0f);

        // The field scrolls just far enough to bring the caret into view, vertically if several lines, else sideways.
        const Bool   Vertical = Data->Multiline;
        const Rect   Inner    = Owner.GetBox(Self).Padding.Inset(Frame);
        const Rect   View(Inner.GetPosition(), Inner.GetPosition() + Target.Get<Scroll>()->View);
        const Real32 Point    = Vertical ? At.GetY() : At.GetX();
        const Real32 Reach    = Point + (Vertical ? Shape.Leading : Wide);
        const Real32 Front    = Vertical ? View.GetMinimumY() : View.GetMinimumX();
        const Real32 Back     = Vertical ? View.GetMaximumY() : View.GetMaximumX();
        const Real32 Before   = Scrollbar::GetOffset(Target, Vertical);
        Real32       Offset   = Before;

        if (Point < Front)
        {
            Offset -= Front - Point;
        }
        else if (Reach > Back)
        {
            Offset += Reach - Back;
        }

        Moved = Scrollbar::ScrollTo(Target, Vertical, Offset) || Moved;

        const Real32  Travel = Before - Scrollbar::GetOffset(Target, Vertical);
        const Vector2 Shift  = Vertical ? Vector2(0.0f, Travel) : Vector2(Travel, 0.0f);

        // The caret stands on its line, centred down it.
        const Real32 Tall = Caret.GetFrame().GetHeight();
        const Real32 Down = At.GetY() + Shift.GetY() + (Shape.Leading - Tall) * 0.5f;
        const Real32 Edge = At.GetX() + Shift.GetX();
        Moved = Put(Caret, Corner, Rect(Edge, Down, Edge + Wide, Down + Tall), false) || Moved;

        // The selection is a band per line it covers, past the line's end where it runs on to the next.
        if (!Selection.IsFixed())
        {
            const UInt   Low   = Selection.GetMinimum();
            const UInt   High  = Selection.GetMaximum();
            const UInt32 First = Ruler::GetLine(Shape, Low);
            const UInt32 Last  = Ruler::GetLine(Shape, High);

            for (UInt32 Line = First; Line <= Last; ++Line)
            {
                const UInt   End   = Ruler::GetEnd(Shape, Content, Line);
                const Real32 Upper = Shape.Top + Shape.Leading * static_cast<Real32>(Line) + Shift.GetY();

                // A line scrolled out of view needs no band, so a long selection costs only what shows.
                if (Upper + Shape.Leading < View.GetMinimumY() || Upper > View.GetMaximumY())
                {
                    continue;
                }

                const Real32 Left  = Ruler::Place(Shape, Content, Max<UInt>(Low, Shape.Starts[Line])).GetX()
                    + Shift.GetX();
                const Real32 Right = Ruler::Place(Shape, Content, Min(High, End)).GetX() + Shift.GetX()
                    + (High > End ? Shape.Size * 0.3f : 0.0f);

                if (!Band.IsAlive())
                {
                    Band = Target.Create();
                    Band.GetTree().SetStyle(Band.GetHandle(), Owner.GetStyle(Caret.GetNext().GetNext().GetHandle()));
                    Band.EditBox().Placement = Position::Absolute;
                }

                Moved = Put(Band, Corner, Rect(Left, Upper, Right, Upper + Shape.Leading), true) || Moved;
                Moved = Show(Band, true) || Moved;
                Band  = Band.GetNext();
            }
        }

        for (; Band.IsAlive(); Band = Band.GetNext())
        {
            Moved = Show(Band, false) || Moved;
        }
        return Moved;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Textbox::OnKey(Ref<Interface> Owner, Element Target, ZyInput::Key Key, Ref<Bool> Edited)
    {
        const Ptr<Entry>     Data      = Target.Get<Entry>();
        const Text           Content   = GetText(Target);
        const Bool           Shift     = HasBit(Owner.GetModifiers(), ZyInput::Modifier::Shift);
        const Bool           Words     = HasBit(Owner.GetModifiers(), ZyInput::Modifier::Control);
        const UInt           Caret     = GetCaret(Target);
        const AnyRange<UInt> Selection = GetSelection(* Data, Content.GetSize());

        switch (Key)
        {
        case ZyInput::Key::Left:
            // An arrow without shift first lets go of the selection, at its end that way.
            if (!Selection.IsFixed() && !Shift)
            {
                Move(Target, Selection.GetMinimum(), false);
            }
            else
            {
                Move(Target, Words ? Unicode::Backward(Content, Caret) : Unicode::Previous(Content, Caret), Shift);
            }
            return true;
        case ZyInput::Key::Right:
            if (!Selection.IsFixed() && !Shift)
            {
                Move(Target, Selection.GetMaximum(), false);
            }
            else
            {
                Move(Target, Words ? Unicode::Forward(Content, Caret) : Unicode::Next(Content, Caret), Shift);
            }
            return true;
        case ZyInput::Key::Home:
        case ZyInput::Key::End:
        {
            const Bool Back = (Key == ZyInput::Key::Home);
            Block      Shape;

            // The ends of the line the caret stands on, or of the whole text with control held or on one line.
            if (Data->Multiline && !Words && Survey(Owner, Target, Shape))
            {
                const UInt32 Line = Ruler::GetLine(Shape, Caret);
                Move(Target, Back ? Shape.Starts[Line] : Ruler::GetEnd(Shape, Content, Line), Shift);
            }
            else
            {
                Move(Target, Back ? 0 : Content.GetSize(), Shift);
            }
            return true;
        }
        case ZyInput::Key::Up:
        case ZyInput::Key::Down:
            if (!Data->Multiline)
            {
                return false;
            }
            Step(Owner, Target, Key == ZyInput::Key::Up ? -1 : 1, Shift);
            return true;
        case ZyInput::Key::PageUp:
        case ZyInput::Key::PageDown:
        {
            if (!Data->Multiline)
            {
                return false;
            }

            // A page is as many lines as the field shows at once.
            Block        Shape;
            const Real32 View = Target.Get<Scroll>()->View.GetY();
            const SInt32 Lines = Survey(Owner, Target, Shape) && Shape.Leading > 0.0f
                ? Max(static_cast<SInt32>(View / Shape.Leading), 1)
                : 1;
            Step(Owner, Target, Key == ZyInput::Key::PageUp ? -Lines : Lines, Shift);
            return true;
        }
        case ZyInput::Key::Enter:
            if (!Data->Multiline)
            {
                return false;
            }
            Edited = Insert(Target, "\n");
            return true;
        case ZyInput::Key::Backspace:
            Edited = Erase(Target, false, Words);
            return true;
        case ZyInput::Key::Delete:
            Edited = Erase(Target, true, Words);
            return true;
        case ZyInput::Key::Space:
            // Space arrives typed, so the key only stays in the field rather than clicking it.
            return true;
        case ZyInput::Key::A:
            if (Words)
            {
                Data->Anchor = 0;
                Data->Caret  = static_cast<UInt32>(Content.GetSize());
            }
            return Words;
        case ZyInput::Key::C:
            if (Words && !Selection.IsFixed())
            {
                Owner.Copy(GetSelected(Target));
            }
            return Words;
        case ZyInput::Key::X:
            if (Words && !Selection.IsFixed())
            {
                Owner.Copy(GetSelected(Target));
                Edited = Replace(Target, Text());
            }
            return Words;
        case ZyInput::Key::V:
            if (Words)
            {
                Edited = Insert(Target, Owner.Paste());
            }
            return Words;
        default:
            return false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Textbox::Commit(Ref<Interface> Owner, Element Target, Bool Always)
    {
        const Ptr<Entry> Data    = Target.Get<Entry>();
        const Text       Content = GetText(Target);
        const Bool       Changed = Content != Data->Original;
        const Bool       Open    = Data->Editing;

        // Settled before reporting, since what hears the commit may edit the field again.
        Data->Original = Content;
        Data->Editing  = false;

        if (Changed || Always)
        {
            Owner.Dispatch(Event(Signal::kCommit, Target.GetHandle()));
        }

        if (Open)
        {
            Owner.Dispatch(Event(Signal::kEnd, Target.GetHandle()));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Textbox::Revert(Ref<Interface> Owner, Element Target)
    {
        const Ptr<Entry> Data    = Target.Get<Entry>();
        const Bool       Changed = GetText(Target) != Data->Original;
        const Bool       Open    = Data->Editing;

        Data->Editing = false;

        // The text put back is reported as a change, so a binding writes it back as well.
        if (Changed)
        {
            SetText(Target, Data->Original);
            Owner.Dispatch(Event(Signal::kChange, Target.GetHandle()));
        }

        if (Open)
        {
            Owner.Dispatch(Event(Signal::kEnd, Target.GetHandle()));
        }

        if (Owner.GetFocused() == Target.GetHandle())
        {
            Owner.Focus(Handle());
        }
        return Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Textbox::OnPress(Ref<Interface> Owner, Element Target, ConstRef<Event> Message)
    {
        const Ptr<Entry> Data    = Target.Get<Entry>();
        const UInt       Offset  = Locate(Owner, Target, Message.Position);
        const Text       Content = GetText(Target);

        if (Message.Count == 2)
        {
            UInt Start = 0;
            UInt End   = 0;
            Unicode::Word(Content, Offset, Start, End);

            Data->Anchor = static_cast<UInt32>(Start);
            Data->Caret  = static_cast<UInt32>(End);
        }
        else if (Message.Count >= 3)
        {
            Data->Anchor = 0;
            Data->Caret  = static_cast<UInt32>(Content.GetSize());
        }
        else
        {
            Move(Target, Offset, HasBit(Owner.GetModifiers(), ZyInput::Modifier::Shift));
        }

        // Only a single press drags the selection out; a word or the whole text stays as picked.
        Data->Selecting = (Message.Count == 1);
        Data->Goal      = -1.0f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Textbox::Insert(Element Target, Text Content)
    {
        const ConstPtr<Entry> Data = Target.Get<Entry>();

        if (Data->ReadOnly)
        {
            return false;
        }

        // The limit counts the characters left once the selection is gone.
        const UInt Kept = Unicode::Count(GetText(Target)) - Unicode::Count(GetSelected(Target));
        const UInt Room = Data->Limit == 0 ? kMaximum<UInt> : Max<UInt>(Data->Limit, Kept) - Kept;

        Str Typed;

        if (Unicode::Clean(Content, Data->Multiline, Room, Typed) == 0)
        {
            return false;
        }

        if (Data->Numeric)
        {
            Str Number;

            for (UInt Index = 0; Index < Typed.GetSize(); ++Index)
            {
                const Char Letter = Typed[Index];

                if ((Letter >= '0' && Letter <= '9') || Letter == '-' || Letter == '.')
                {
                    Number.Append(Letter);
                }
            }
            Typed = ::Move(Number);
        }
        return !Typed.IsEmpty() && Replace(Target, Typed);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Textbox::Erase(Element Target, Bool Ahead, Bool Words)
    {
        const Ptr<Entry> Data    = Target.Get<Entry>();
        const Text       Content = GetText(Target);

        if (Data->ReadOnly)
        {
            return false;
        }

        if (Data->Caret != Data->Anchor)
        {
            return Replace(Target, Text());
        }

        const UInt Caret = GetCaret(Target);
        const UInt From  = Ahead ? Caret
            : (Words ? Unicode::Backward(Content, Caret) : Unicode::Previous(Content, Caret));
        const UInt To    = Ahead ? (Words ? Unicode::Forward(Content, Caret) : Unicode::Next(Content, Caret)) : Caret;

        if (From == To)
        {
            return false;
        }

        Data->Anchor = static_cast<UInt32>(From);
        Data->Caret  = static_cast<UInt32>(To);
        return Replace(Target, Text());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Textbox::Replace(Element Target, Text Content)
    {
        const Ptr<Entry>     Data      = Target.Get<Entry>();
        const AnyRange<UInt> Selection = GetSelection(* Data, GetText(Target).GetSize());

        if (Data->ReadOnly || (Selection.IsFixed() && Content.IsEmpty()))
        {
            return false;
        }

        // The text is edited where it stands, so only the change is written.
        Ref<Str> Kept = Target.GetFirst().Edit<Caption>()->Content;

        if (!Selection.IsFixed())
        {
            Kept.Remove(Selection.GetMinimum(), Selection.GetLength());
        }
        Kept.Insert(Selection.GetMinimum(), Content);

        Data->Caret  = static_cast<UInt32>(Selection.GetMinimum() + Content.GetSize());
        Data->Anchor = Data->Caret;
        Data->Goal   = -1.0f;

        Refresh(Target);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Textbox::Move(Element Target, UInt Offset, Bool Extend)
    {
        const Ptr<Entry> Data = Target.Get<Entry>();

        Data->Caret = static_cast<UInt32>(Min<UInt>(Offset, GetText(Target).GetSize()));
        Data->Goal  = -1.0f;

        if (!Extend)
        {
            Data->Anchor = Data->Caret;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Textbox::Step(Ref<Interface> Owner, Element Target, SInt32 Lines, Bool Extend)
    {
        const Ptr<Entry> Data    = Target.Get<Entry>();
        const Text       Content = GetText(Target);
        Block            Shape;

        if (!Survey(Owner, Target, Shape))
        {
            return;
        }

        const UInt   Caret = GetCaret(Target);
        const UInt32 Line  = Ruler::GetLine(Shape, Caret);
        const SInt64 Aim   = static_cast<SInt64>(Line) + Lines;
        const UInt32 Count = static_cast<UInt32>(Shape.Starts.GetSize());

        // The caret keeps aiming at how far along its line it first stood, so a short line passed over loses nothing.
        if (Data->Goal < 0.0f)
        {
            const Real32 Margin = Ruler::Lead(Shape.Alignment, Shape.Inner, Shape.Widths[Line]);
            Data->Goal = Ruler::Place(Shape, Content, Caret).GetX() - Margin;
        }

        const Real32 Goal = Data->Goal;
        UInt         Offset;

        if (Aim < 0)
        {
            Offset = 0;
        }
        else if (Aim >= static_cast<SInt64>(Count))
        {
            Offset = Content.GetSize();
        }
        else
        {
            const UInt32 Next  = static_cast<UInt32>(Aim);
            const Real32 Lead  = Ruler::Lead(Shape.Alignment, Shape.Inner, Shape.Widths[Next]);
            const Real32 Depth = Shape.Top + Shape.Leading * (static_cast<Real32>(Next) + 0.5f);
            Offset = Ruler::Find(Shape, Content, Vector2(Lead + Goal, Depth));
        }

        Move(Target, Offset, Extend);
        Data->Goal = Goal;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Textbox::Survey(Ref<Interface> Owner, Element Target, Ref<Block> Output)
    {
        const ConstPtr<Entry> Data    = Target.Get<Entry>();
        const Element         Shown   = Target.GetFirst();
        const Text            Content = GetText(Target);

        // An empty field shows its hint in place of its text, so the caret stands where the hint starts.
        const Element           Holder = Content.IsEmpty() ? Shown.GetNext().GetNext() : Shown;
        const ConstPtr<Caption> Words  = Holder.Get<Caption>();

        if (!Words)
        {
            return false;
        }

        const Bool Loaded = Words->Font && Words->Font->HasCompleted();

        Output.Inner     = Owner.GetBox(Holder.GetHandle()).Padding.Inset(Holder.GetFrame());
        Output.Size      = Words->Size;
        Output.Alignment = Words->Alignment;
        Output.Source    = Loaded ? AddressOf(Owner.GetFace(Words->Font)) : nullptr;
        Output.Leading   = Loaded ? Words->Font->GetLineHeight(Words->Size) : Words->Size;

        // The lines break where the layout broke them, at the width the text was given.
        Ruler::Break(Content, Data->Multiline ? Output.Inner.GetWidth() : 0.0f, Output);

        const Real32 Tall = Output.Leading * static_cast<Real32>(Output.Starts.GetSize());
        Output.Top = Output.Inner.GetMinimumY() + (Output.Inner.GetHeight() - Tall) * 0.5f;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt Textbox::Locate(Ref<Interface> Owner, Element Target, Vector2 Point)
    {
        Block Shape;
        return Survey(Owner, Target, Shape) ? Ruler::Find(Shape, GetText(Target), Point) : GetCaret(Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Textbox::Put(Element Part, Vector2 Corner, Rect Box, Bool Sized)
    {
        ConstRef<UI::Box> Now    = Part.GetTree().GetBox(Part.GetHandle());
        const Real32      Left   = Box.GetMinimumX() - Corner.GetX();
        const Real32      Top    = Box.GetMinimumY() - Corner.GetY();
        const Real32      Width  = Max(Box.GetWidth(), 0.0f);
        const Real32      Height = Max(Box.GetHeight(), 0.0f);

        const Bool Placed = Near(Now.Left, Left) && Near(Now.Top, Top);
        const Bool Fitted = !Sized || (Near(Now.Width, Width) && Near(Now.Height, Height));

        if (Placed && Fitted)
        {
            return false;
        }

        Ref<UI::Box> Shape = Part.EditBox();
        Shape.Left = Length::Pixels(Left);
        Shape.Top  = Length::Pixels(Top);

        if (Sized)
        {
            Shape.Width  = Length::Pixels(Width);
            Shape.Height = Length::Pixels(Height);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Textbox::Show(Element Part, Bool Shown)
    {
        if (Part.HasState(State::Hidden) != Shown)
        {
            return false;
        }
        Part.SetState(State::Hidden, !Shown);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Textbox::Refresh(Element Target)
    {
        const Bool    Empty = GetText(Target).IsEmpty();
        const Element Shown = Target.GetFirst();
        const Element Hint  = Shown.GetNext().GetNext();

        // The text and the hint take turns in the same place, so the hint lays out exactly where the text will.
        Show(Shown, !Empty);
        Show(Hint, Empty);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    AnyRange<UInt> Textbox::GetSelection(ConstRef<Entry> Data, UInt Size)
    {
        return AnyRange(Min<UInt>(Data.Caret, Size), Min<UInt>(Data.Anchor, Size));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Textbox::Near(ConstRef<Length> Value, Real32 Pixels)
    {
        // A part stands where it should within a hundredth of a pixel, so rounding never lays the pane out again.
        return Value.Measure == Length::Unit::Pixel && Abs(Value.Value - Pixels) < 0.01f;
    }
}