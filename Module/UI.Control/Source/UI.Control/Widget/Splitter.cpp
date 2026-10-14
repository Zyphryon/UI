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

#include "Splitter.hpp"
#include "Common.hpp"
#include "Panel.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Layout/Virtual.hpp"
#include "UI.Core/Facet/Visual/Look.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Markup/Markup.hpp"
#include "UI.Markup/Parser.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Splitter::Create(Element Parent, Direction Flow, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style, Flow);
        Widget::Govern(Result, Handler::Create<&Splitter::OnEvent>(), false);

        // It takes the room it is given, never what its panes ask.
        Ref<Box> Shape = Result.EditBox();
        Shape.Grow  = 1.0f;
        Shape.Basis = Length::Pixels(0.0f);

        Result.Add<Split>();
        Result.Add<Virtual>().Realize = Realizer::Create<&Splitter::OnRealize>();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Splitter::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Text      Style  = Markup::GetText(Node, "Style", "Splitter");
        const Direction Flow   = Markup::GetText(Node, "Orientation") == "Vertical"
            ? Direction::Column : Direction::Row;
        const Element   Result = Create(Parent, Flow, Style);

        Result.Get<Split>()->Name = Markup::GetText(Node, "Name");

        const ConstPtr<JsonValue> Panes = Markup::GetField(Node, "Panes");

        if (!Panes || !Panes->IsArray())
        {
            if (Panes)
            {
                LOG_W("A splitter's 'Panes' must be a list of panes, so it holds none");
            }
            return Result;
        }

        for (ConstRef<JsonValue> Entry : Panes->GetArray())
        {
            Length Size;

            const ConstPtr<JsonValue> Given = Markup::GetField(Entry, "Size");

            if (Given && !Parser::ParseLength(* Given, Size))
            {
                LOG_W("A splitter's pane 'Size' must be pixels or a share as '25%', so it shares what is left");
            }

            const Real32  Floor = Markup::GetNumber(Entry, "Minimum", kMinimum);
            const Element Pane  = AddPane(Result, Size, Floor, Style);

            if (const Text Name = Markup::GetText(Entry, "Name"); !Name.IsEmpty())
            {
                Pane.SetName(Name);
            }

            // A pane hidden, as a sidebar toggled off, gives its room to the others and takes its bar with it.
            Pane.SetState(State::Hidden, Markup::GetBool(Entry, "Hidden"));
            Reader.Bind(Pane, Entry, "Hidden", Binder::PutState<State::Hidden>());

            const ConstPtr<JsonValue> Children = Markup::GetField(Entry, "Children");

            if (Children && Children->IsArray())
            {
                for (ConstRef<JsonValue> Child : Children->GetArray())
                {
                    Reader.Build(Pane, Child);
                }
            }
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Splitter::AddPane(Element Owner, Real32 Size, Real32 Minimum, Text Style)
    {
        return AddPane(Owner, Size > 0.0f ? Length::Pixels(Size) : Length::Auto(), Minimum, Style);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Splitter::AddPane(Element Owner, Length Size, Real32 Minimum, Text Style)
    {
        const Bool   Side  = (Owner.GetTree().GetBox(Owner.GetHandle()).Flow == Direction::Row);
        const Real32 Least = Max(Minimum, 0.0f);

        // A bar stands between each two panes, a control of its own so a press on it is held by it.
        if (GetCount(Owner) > 0)
        {
            const Element Bar = Owner.Create();
            Widget::Govern(Bar, Handler(), false);
            Bar.SetStyle(Widget::Part(Style, "Bar"));

            Ref<Box> Shape = Bar.EditBox();
            (Side ? Shape.Width : Shape.Height) = Length::Pixels(kThickness);
            Shape.Shrink = 0.0f;
        }

        // A share is kept as one, measured against the room; any other size is pixels, and none shares what is left.
        const Bool    Spreads = Size.Measure == Length::Unit::Percent && Size.Value > 0.0f;
        const Element Pane    = Panel::Create(Owner, Widget::Part(Style, "Pane"));
        Pane.Add<Look>().Clip = true;

        if (Spreads)
        {
            Spread(Pane, Size.Value);
        }
        else
        {
            Shape(Pane, Size.Measure == Length::Unit::Pixel ? Max(Size.Value, 0.0f) : 0.0f, Least);
        }

        Ref<Split> Own = * Owner.Get<Split>();
        Own.Minimums.Append(Least);
        Own.Folded.Append(-1.0f);
        Own.Shares.Append(Spreads ? Size.Value : 0.0f);
        return Pane;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Splitter::GetCount(Element Owner)
    {
        UInt32 Count = 0;

        for (Element Child = Owner.GetFirst(); Child.IsAlive(); Child = Child.GetNext())
        {
            Count += !IsBar(Child) ? 1 : 0;
        }
        return Count;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Splitter::GetPane(Element Owner, UInt32 Index)
    {
        UInt32 Count = 0;

        for (Element Child = Owner.GetFirst(); Child.IsAlive(); Child = Child.GetNext())
        {
            if (IsBar(Child))
            {
                continue;
            }

            if (Count++ == Index)
            {
                return Child;
            }
        }
        return Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Splitter::GetBar(Element Owner, UInt32 Index)
    {
        UInt32 Count = 0;

        for (Element Child = Owner.GetFirst(); Child.IsAlive(); Child = Child.GetNext())
        {
            if (IsBar(Child) && ++Count == Index)
            {
                return Child;
            }
        }
        return Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splitter::SetSize(Element Owner, UInt32 Index, Real32 Size)
    {
        const Element    Pane = GetPane(Owner, Index);
        const Ptr<Split> Own  = Owner.Get<Split>();

        if (Pane.IsAlive() && Own && Index < Own->Minimums.GetSize())
        {
            Own->Folded[Index] = -1.0f;
            Own->Shares[Index] = 0.0f;
            Shape(Pane, Max(Size, 0.0f), Own->Minimums[Index]);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Splitter::GetSize(Element Owner, UInt32 Index)
    {
        const Element Pane = GetPane(Owner, Index);
        return Pane.IsAlive() && !Owner.GetTree().IsPending(Pane.GetHandle()) ? Along(Owner, Pane.GetFrame()) : 0.0f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splitter::SetShare(Element Owner, UInt32 Index, Real32 Share)
    {
        const Element    Pane = GetPane(Owner, Index);
        const Ptr<Split> Own  = Owner.Get<Split>();

        if (!Pane.IsAlive() || !Own || Index >= Own->Shares.GetSize())
        {
            return;
        }

        Own->Folded[Index] = -1.0f;
        Own->Shares[Index] = Max(Share, 0.0f);

        if (Share > 0.0f)
        {
            Spread(Pane, Share);
        }
        else
        {
            Shape(Pane, 0.0f, Own->Minimums[Index]);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Splitter::GetShare(Element Owner, UInt32 Index)
    {
        const ConstPtr<Split> Own = Owner.Get<Split>();
        return Own && Index < Own->Shares.GetSize() ? Own->Shares[Index] : 0.0f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Length Splitter::GetMeasure(Element Owner, UInt32 Index)
    {
        const Element         Pane = GetPane(Owner, Index);
        const ConstPtr<Split> Own  = Owner.Get<Split>();

        if (!Pane.IsAlive() || !Own || Index >= Own->Shares.GetSize())
        {
            return Length::Auto();
        }

        if (Own->Shares[Index] > 0.0f)
        {
            return Length::Percent(Own->Shares[Index]);
        }

        // A folded pane keeps the size it had in the fold, its box laid out at nothing meanwhile.
        if (const Real32 Folded = Own->Folded[Index]; Folded >= 0.0f)
        {
            return Folded > 0.0f ? Length::Pixels(Folded) : Length::Auto();
        }
        return IsSharing(Pane) ? Length::Auto() : Length::Pixels(Owner.GetTree().GetBox(Pane.GetHandle()).Basis.Value);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Splitter::Save(Element Scope)
    {
        JsonValue Root;
        Root.SetObject();
        Gather(Scope, JsonObject(Root));
        return JsonDocument::Dump(Root);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Splitter::Load(Element Scope, Text Source, Ref<Str> Error)
    {
        JsonValue Root;

        if (!Parser::Parse(Source, Root, Error))
        {
            return false;
        }

        Scatter(Scope, Root);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splitter::SetFolded(Element Owner, UInt32 Index, Bool Folded)
    {
        const Element    Pane = GetPane(Owner, Index);
        const Ptr<Split> Own  = Owner.Get<Split>();

        if (!Pane.IsAlive() || !Own || Index >= Own->Folded.GetSize() || (Own->Folded[Index] >= 0.0f) == Folded)
        {
            return;
        }

        const Real32 Least = Own->Minimums[Index];

        if (Folded)
        {
            // A pane sharing what is left opens sharing again; one keeping a size or a share opens at it.
            const Real32 Kept = Owner.GetTree().GetBox(Pane.GetHandle()).Basis.Value;
            Own->Folded[Index] = IsSharing(Pane) || Own->Shares[Index] > 0.0f ? 0.0f : Max(Kept, Least);

            Ref<Box> Edit = Pane.EditBox();
            Edit.Basis = Length::Pixels(0.0f);
            Edit.Grow  = 0.0f;
        }
        else
        {
            const Real32 Size = Own->Folded[Index];
            Own->Folded[Index] = -1.0f;

            if (const Real32 Share = Own->Shares[Index]; Share > 0.0f)
            {
                Spread(Pane, Share);
            }
            else
            {
                Shape(Pane, Size, Least);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Splitter::IsFolded(Element Owner, UInt32 Index)
    {
        const ConstPtr<Split> Own = Owner.Get<Split>();
        return Own && Index < Own->Folded.GetSize() && Own->Folded[Index] >= 0.0f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Splitter::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        // Only its own bars answer; whatever a pane holds belongs to that pane.
        const Bool Foreign = Message.Target == Self || Owner.GetParent(Message.Target) != Self;

        if (Foreign || !IsBar(Element(Owner, Message.Target)))
        {
            return false;
        }

        const Element Root(Owner, Self);
        const Element Bar(Owner, Message.Target);

        switch (Message.Kind)
        {
        case Signal::kPress:
        {
            if (Message.Button != ZyInput::Button::Left)
            {
                return false;
            }

            if (Message.Count == 2)
            {
                if (Fold(Root, Bar))
                {
                    Owner.Dispatch(Event(Signal::kChange, Self));
                }
                return true;
            }

            const Ptr<Split> Own = Owner.Edit<Split>(Self);
            Own->Grab = Toward(Root, Message.Position) - Start(Root, Bar.GetFrame());
            Own->Held = true;
            Owner.Dispatch(Event(Signal::kBegin, Self));
            return true;
        }
        case Signal::kDrag:
        {
            if (Drag(Root, Bar, Message.Position))
            {
                Owner.Dispatch(Event(Signal::kChange, Self));
            }
            return true;
        }
        case Signal::kRelease:
        {
            // Only a hold the press took ends, so the release after a double click reports nothing.
            if (const Ptr<Split> Own = Owner.Edit<Split>(Self); Own->Held)
            {
                Own->Held = false;
                Owner.Dispatch(Event(Signal::kEnd, Self));
                return true;
            }
            return false;
        }
        default:
            return false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Splitter::OnRealize(Ref<Interface> Owner, Handle Self)
    {
        const Element Root(Owner, Self);
        Bool          Changed = false;
        Bool          Shown   = false;

        // A bar shows only after a shown pane and before another, so a hidden pane never leaves two bars side by side.
        for (Element Child = Root.GetFirst(); Child.IsAlive(); Child = Child.GetNext())
        {
            if (!IsBar(Child))
            {
                Shown = !Child.HasState(State::Hidden);
                continue;
            }

            Bool Later = false;

            for (Element Next = Child.GetNext(); Next.IsAlive() && !Later; Next = Next.GetNext())
            {
                Later = !IsBar(Next) && !Next.HasState(State::Hidden);
            }

            const Bool Hidden = !(Shown && Later);

            if (Child.HasState(State::Hidden) != Hidden)
            {
                Child.SetState(State::Hidden, Hidden);
                Changed = true;
            }
        }
        return Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Splitter::Drag(Element Root, Element Bar, Vector2 Point)
    {
        SInt32 First  = -1;
        SInt32 Second = -1;
        Flank(Root, Bar, First, Second);

        const Ptr<Split> Own = Root.Get<Split>();

        if (First < 0 || Second < 0 || !Own)
        {
            return false;
        }

        const Element Before = GetPane(Root, static_cast<UInt32>(First));
        const Element After  = GetPane(Root, static_cast<UInt32>(Second));
        const Real32  Near   = Along(Root, Before.GetFrame());
        const Real32  Far    = Along(Root, After.GetFrame());

        // The bar follows the pointer, held where it was pressed, as far as both panes' least sizes allow.
        const Real32 Pointer = Toward(Root, Point);
        const Real32 Wanted  = Pointer - Own->Grab - Start(Root, Bar.GetFrame());
        const Real32 Lowest  = Min(Own->Minimums[First] - Near, 0.0f);
        const Real32 Highest = Max(Far - Own->Minimums[Second], 0.0f);
        const Real32 Delta   = Clamp(Wanted, Lowest, Highest);

        if (Abs(Delta) < 0.5f)
        {
            return false;
        }

        const Bool Sharing = IsSharing(Before);
        const Bool Yields  = IsSharing(After);

        // A pane dragged open from folded is open again.
        Own->Folded[First]  = -1.0f;
        Own->Folded[Second] = -1.0f;

        // A pane keeping a size takes the room and a sharing one gives way; of two sharing, the first keeps it.
        if (!Sharing || Yields)
        {
            Resize(Root, static_cast<UInt32>(First), Max(Near + Delta, 1.0f));
        }

        if (!Yields)
        {
            Resize(Root, static_cast<UInt32>(Second), Max(Far - Delta, 1.0f));
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Splitter::Fold(Element Root, Element Bar)
    {
        SInt32 First  = -1;
        SInt32 Second = -1;
        Flank(Root, Bar, First, Second);

        if (First < 0 || Second < 0)
        {
            return false;
        }

        // A pane folded beside the bar opens again; else the one keeping a size folds, or the smaller of two.
        const Bool Near = IsFolded(Root, static_cast<UInt32>(First));

        if (Near || IsFolded(Root, static_cast<UInt32>(Second)))
        {
            SetFolded(Root, static_cast<UInt32>(Near ? First : Second), false);
            return true;
        }

        const Element Before  = GetPane(Root, static_cast<UInt32>(First));
        const Element After   = GetPane(Root, static_cast<UInt32>(Second));
        const Bool    Sharing = IsSharing(Before);

        SInt32 Folded = First;

        if (Sharing != IsSharing(After))
        {
            Folded = Sharing ? Second : First;
        }
        else if (Along(Root, After.GetFrame()) < Along(Root, Before.GetFrame()))
        {
            Folded = Second;
        }
        SetFolded(Root, static_cast<UInt32>(Folded), true);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splitter::Gather(Element Node, JsonObject Output)
    {
        if (const ConstPtr<Split> Own = Node.Get<Split>(); Own && !Own->Name.IsEmpty())
        {
            const UInt32 Count = GetCount(Node);
            JsonObject   Entry = Output.SetObject(Own->Name);
            JsonArray    Sizes = Entry.SetArray("Sizes");

            // A size in pixels is a number, a share a percent, and a pane sharing what is left is `Auto`.
            for (UInt32 Index = 0; Index < Count; ++Index)
            {
                const Length Measure = GetMeasure(Node, Index);

                if (Measure.Measure == Length::Unit::Pixel)
                {
                    Sizes.AddNumber(Measure.Value);
                }
                else if (Measure.Measure == Length::Unit::Percent)
                {
                    Str Share;
                    Share.AppendReal(Measure.Value * 100.0f, 3);
                    Share.Append('%');
                    Sizes.AddString(Share);
                }
                else
                {
                    Sizes.AddString("Auto");
                }
            }

            // Added once the sizes are in, since adding a field may move the one added before it.
            JsonArray Folded = Entry.SetArray("Folded");

            for (UInt32 Index = 0; Index < Count; ++Index)
            {
                if (IsFolded(Node, Index))
                {
                    Folded.AddNumber(Index);
                }
            }
        }

        for (Element Child = Node.GetFirst(); Child.IsAlive(); Child = Child.GetNext())
        {
            Gather(Child, Output);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splitter::Scatter(Element Node, ConstRef<JsonValue> Source)
    {
        const ConstPtr<Split>     Own   = Node.Get<Split>();
        const ConstPtr<JsonValue> Entry = Own && !Own->Name.IsEmpty() ? Parser::Find(Source, Own->Name) : nullptr;

        const ConstPtr<JsonValue> Sizes = Entry ? Parser::Find(* Entry, "Sizes") : nullptr;

        if (Sizes && Sizes->IsArray())
        {
            ConstRef<JsonValue::Array> List  = Sizes->GetArray();
            const UInt32               Count = Min(GetCount(Node), static_cast<UInt32>(List.GetSize()));

            // Every pane given a size is opened, then the ones listed folded fold again.
            for (UInt32 Index = 0; Index < Count; ++Index)
            {
                if (Length Measure; !Parser::ParseLength(List[Index], Measure))
                {
                    LOG_W("A splitter's saved size '{0}' is no size, so the pane keeps its own", Index);
                }
                else if (Measure.Measure == Length::Unit::Percent)
                {
                    SetShare(Node, Index, Measure.Value);
                }
                else
                {
                    SetSize(Node, Index, Measure.Measure == Length::Unit::Pixel ? Measure.Value : 0.0f);
                }
            }

            if (const ConstPtr<JsonValue> Folded = Parser::Find(* Entry, "Folded"); Folded && Folded->IsArray())
            {
                for (ConstRef<JsonValue> Index : Folded->GetArray())
                {
                    if (Real32 Number = 0.0f; Parser::ParseNumber(Index, Number) && Number >= 0.0f)
                    {
                        SetFolded(Node, static_cast<UInt32>(Number), true);
                    }
                }
            }
        }

        for (Element Child = Node.GetFirst(); Child.IsAlive(); Child = Child.GetNext())
        {
            Scatter(Child, Source);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splitter::Resize(Element Root, UInt32 Index, Real32 Size)
    {
        const Ptr<Split> Own  = Root.Get<Split>();
        const Element    Pane = GetPane(Root, Index);
        const Real32     Room = GetRoom(Root);

        if (Own->Shares[Index] > 0.0f && Room > 0.0f)
        {
            Own->Shares[Index] = Size / Room;
            Spread(Pane, Own->Shares[Index]);
        }
        else
        {
            Shape(Pane, Size, Own->Minimums[Index]);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splitter::Spread(Element Pane, Real32 Share)
    {
        Ref<Box> Edit = Pane.EditBox();
        Edit.Basis  = Length::Percent(Share);
        Edit.Grow   = 0.0f;
        Edit.Shrink = 0.0f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Splitter::GetRoom(Element Root)
    {
        ConstRef<Box> Shape   = Root.GetTree().GetBox(Root.GetHandle());
        const Real32  Padding = Shape.Flow == Direction::Row
            ? Shape.Padding.Left + Shape.Padding.Right
            : Shape.Padding.Top + Shape.Padding.Bottom;
        return Max(Along(Root, Root.GetFrame()) - Padding, 0.0f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splitter::Flank(Element Root, Element Bar, Ref<SInt32> Before, Ref<SInt32> After)
    {
        SInt32    Index  = 0;
        Bool      Passed = false;

        Before = -1;
        After  = -1;

        for (Element Child = Root.GetFirst(); Child.IsAlive(); Child = Child.GetNext())
        {
            if (Child == Bar)
            {
                Passed = true;
                continue;
            }

            if (IsBar(Child))
            {
                continue;
            }

            if (!Child.HasState(State::Hidden))
            {
                if (!Passed)
                {
                    Before = Index;
                }
                else if (After < 0)
                {
                    After = Index;
                }
            }
            ++Index;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splitter::Shape(Element Pane, Real32 Size, Real32 Minimum)
    {
        // Neither kind shrinks: one keeping a size keeps it, and one sharing what is left starts from its least size.
        Ref<Box> Edit = Pane.EditBox();
        Edit.Shrink = 0.0f;

        if (Size > 0.0f)
        {
            Edit.Basis = Length::Pixels(Max(Size, Minimum));
            Edit.Grow  = 0.0f;
        }
        else
        {
            Edit.Basis = Length::Pixels(Minimum);
            Edit.Grow  = 1.0f;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Splitter::IsSharing(Element Pane)
    {
        return Pane.GetTree().GetBox(Pane.GetHandle()).Grow > 0.0f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Splitter::IsBar(Element Child)
    {
        // A bar is governed so the pointer can take hold of it; a pane is a plain panel.
        return Child.Has<Control>();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Splitter::Along(Element Root, Rect Frame)
    {
        return Root.GetTree().GetBox(Root.GetHandle()).Flow == Direction::Row ? Frame.GetWidth() : Frame.GetHeight();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Splitter::Start(Element Root, Rect Frame)
    {
        return Root.GetTree().GetBox(Root.GetHandle()).Flow == Direction::Row
            ? Frame.GetMinimumX() : Frame.GetMinimumY();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Splitter::Toward(Element Root, Vector2 Point)
    {
        return Root.GetTree().GetBox(Root.GetHandle()).Flow == Direction::Row ? Point.GetX() : Point.GetY();
    }
}