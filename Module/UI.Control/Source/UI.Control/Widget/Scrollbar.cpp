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

#include "Scrollbar.hpp"
#include "Common.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Layout/Virtual.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scrollbar::Create(Element Parent, Bool Vertical, Text Style)
    {
        const Element Result = Parent.Create();

        // Only the thickness is set; the length comes from the parent stretching the bar.
        Ref<Box> Shape = Result.EditBox();
        Shape.Flow = Vertical ? Direction::Column : Direction::Row;
        (Vertical ? Shape.Width : Shape.Height) = Length::Pixels(kThickness);

        Widget::Govern(Result, Handler::Create<&Scrollbar::OnEvent>(), false);
        Result.SetStyle(Style);

        // The bar places the thumb itself after every layout.
        const Element Thumb = Result.Create();
        Widget::Govern(Thumb, Handler::Create<&Scrollbar::OnThumb>(), false);
        Thumb.SetStyle(Widget::Part(Style, "Thumb"));
        Thumb.EditBox().Placement = Position::Absolute;

        Ref<Rail> Own = Result.Add<Rail>();
        Own.Thumb    = Thumb.GetHandle();
        Own.Vertical = Vertical;

        Result.Add<Virtual>().Realize = Realizer::Create<&Scrollbar::OnRealize>();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scrollbar::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Text Way = Markup::GetText(Node, "Orientation", "Vertical");

        if (Way != "Vertical" && Way != "Horizontal")
        {
            LOG_W("A scroll bar's 'Orientation' must be 'Vertical' or 'Horizontal', not '{0}', so it runs up and down",
                  Way);
        }

        const Element Result = Create(Parent, Way != "Horizontal", Markup::GetText(Node, "Style", "Scrollbar"));

        // The element it scrolls may be made after it, so it is looked up by name once the layout stands.
        if (const Text Name = Markup::GetText(Node, "Target"); !Name.IsEmpty())
        {
            Result.Get<Rail>()->Name = Identify(Name);
        }

        SetAutomatic(Result, Markup::GetBool(Node, "Automatic"));
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scrollbar::SetTarget(Element Bar, Element Target)
    {
        if (const Ptr<Rail> Own = Bar.Get<Rail>())
        {
            Own->Target = Target.GetHandle();
            Own->Name   = 0;
        }
        Prepare(Bar, Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scrollbar::GetTarget(Element Bar)
    {
        const Ptr<Rail> Own = Bar.Get<Rail>();

        if (!Own)
        {
            return Element();
        }

        Ref<Tree> Owner = Bar.GetTree();

        // A name given in a layout is looked up once the bar stands in its pane, then kept as a handle.
        if (!Owner.IsAlive(Own->Target) && Own->Name != 0 && !Owner.IsPending(Bar.GetHandle()))
        {
            const Element Found(Owner, Owner.Find(Owner.GetRoot(Bar.GetHandle()), Own->Name));

            if (Found.IsAlive())
            {
                Own->Target = Found.GetHandle();
                Prepare(Bar, Found);
            }
        }
        return Owner.IsAlive(Own->Target) ? Element(Owner, Own->Target) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scrollbar::SetAutomatic(Element Bar, Bool Automatic)
    {
        if (const Ptr<Rail> Own = Bar.Get<Rail>())
        {
            Own->Automatic = Automatic;
        }

        // A bar always shown is shown at once; one hiding itself finds out at the next update.
        if (!Automatic && Bar.HasState(State::Hidden))
        {
            Bar.SetState(State::Hidden, false);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scrollbar::GetThumb(Element Bar)
    {
        const ConstPtr<Rail> Own = Bar.Get<Rail>();
        return Own ? Element(Bar.GetTree(), Own->Thumb) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Scrollbar::ScrollTo(Element Target, Bool Vertical, Real32 Offset)
    {
        const ConstPtr<Scroll> Found = Target.Get<Scroll>();

        if (!Found)
        {
            return false;
        }

        const Real32 Limit = GetLimit(Target, Vertical);
        const Real32 Moved = Clamp(Offset, 0.0f, Limit);

        // Edited only when it moves, since an edit lays the pane out again.
        if (Moved == (Vertical ? Found->Offset : Found->OffsetX))
        {
            return false;
        }

        const Ptr<Scroll> Roll = Target.Edit<Scroll>();

        if (Vertical)
        {
            Roll->Offset = Moved;
            Roll->Follow = Roll->Follow && Moved >= Limit;
        }
        else
        {
            Roll->OffsetX = Moved;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Scrollbar::GetOffset(Element Target, Bool Vertical)
    {
        const ConstPtr<Scroll> Found = Target.Get<Scroll>();
        return Found ? (Vertical ? Found->Offset : Found->OffsetX) : 0.0f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Scrollbar::GetLimit(Element Target, Bool Vertical)
    {
        const ConstPtr<Scroll> Found = Target.Get<Scroll>();

        if (!Found || (!Vertical && !Found->Horizontal))
        {
            return 0.0f;
        }
        return Max((Vertical ? Found->Overflow : Found->OverflowX) - GetView(Target, Vertical), 0.0f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Scrollbar::GetView(Element Target, Bool Vertical)
    {
        const ConstPtr<Scroll> Found = Target.Get<Scroll>();
        return Found ? (Vertical ? Found->View.GetY() : Found->View.GetX()) : 0.0f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Scrollbar::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const Element Bar(Owner, Self);
        const Element Target   = GetTarget(Bar);
        const Bool    Vertical = Bar.Get<Rail>()->Vertical;

        if (!Target.IsAlive())
        {
            return false;
        }

        // The wheel over a bar scrolls along it, a sideways bar taking an upward turn as one to the left.
        if (Message.Kind == Signal::kWheel)
        {
            const Real32 Across = Message.Delta.GetX() != 0.0f ? Message.Delta.GetX() : -Message.Delta.GetY();
            const Real32 Turn   = Vertical ? -Message.Delta.GetY() : Across;
            ScrollTo(Target, Vertical, GetOffset(Target, Vertical) + Turn * kNotch);
            return true;
        }

        // A press on the track itself, not its thumb, moves a page toward the pointer.
        if (Message.Kind != Signal::kPress || Message.Target != Self || Message.Button != ZyInput::Button::Left)
        {
            return false;
        }

        const Rect   Knob  = GetThumb(Bar).GetFrame();
        const Real32 Point = Vertical ? Message.Position.GetY() : Message.Position.GetX();
        const Real32 Start = Vertical ? Knob.GetMinimumY() : Knob.GetMinimumX();
        const Real32 Page  = GetView(Target, Vertical);

        ScrollTo(Target, Vertical, GetOffset(Target, Vertical) + (Point < Start ? -Page : Page));
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Scrollbar::OnThumb(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        if ((Message.Kind != Signal::kPress && Message.Kind != Signal::kDrag) || Message.Target != Self)
        {
            return false;
        }

        const Element   Bar = Element(Owner, Self).GetParent();
        const Ptr<Rail> Own = Bar.Get<Rail>();

        if (!Own)
        {
            return false;
        }

        const Bool   Vertical = Own->Vertical;
        const Rect   Knob     = Owner.GetFrame(Self);
        const Real32 Point    = Vertical ? Message.Position.GetY() : Message.Position.GetX();

        // Taken where it was pressed, so dragging moves it by the pointer's travel rather than snapping it there.
        if (Message.Kind == Signal::kPress)
        {
            Own->Grab = Point - (Vertical ? Knob.GetMinimumY() : Knob.GetMinimumX());
            return true;
        }

        const Element Target = GetTarget(Bar);
        const Rect    Track  = GetTrack(Bar);
        const Real32  Travel = Vertical ? Track.GetHeight() - Knob.GetHeight() : Track.GetWidth() - Knob.GetWidth();

        if (Target.IsAlive() && Travel > 0.0f)
        {
            const Real32 Start = Point - Own->Grab - (Vertical ? Track.GetMinimumY() : Track.GetMinimumX());
            ScrollTo(Target, Vertical, Remap(Start, 0.0f, Travel, 0.0f, GetLimit(Target, Vertical)));
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Scrollbar::OnRealize(Ref<Interface> Owner, Handle Self)
    {
        const Element          Bar(Owner, Self);
        const Element          Target = GetTarget(Bar);
        const ConstPtr<Rail>   Own    = Bar.Get<Rail>();
        const ConstPtr<Scroll> Roll   = Target.Get<Scroll>();

        if (!Own || !Target.IsAlive() || !Roll || Owner.IsPending(Target.GetHandle()))
        {
            return false;
        }

        const Bool   Vertical = Own->Vertical;
        const Real32 View     = GetView(Target, Vertical);
        const Real32 Content  = Vertical ? Roll->Overflow : Roll->OverflowX;
        const Real32 Limit    = GetLimit(Target, Vertical);
        Bool         Changed  = false;

        // Showing a bar narrows the view, so the next pass may show the other bar too but never hides this one.
        if (Own->Automatic)
        {
            const Bool Fits = Limit <= 0.5f;

            if (Bar.HasState(State::Hidden) != Fits)
            {
                Bar.SetState(State::Hidden, Fits);
                Changed = true;
            }
        }

        if (Bar.HasState(State::Hidden))
        {
            return Changed;
        }

        // The thumb is as long against the track as the view against the content, and stands where the offset does.
        const Rect   Track = GetTrack(Bar);
        const Real32 Run   = Vertical ? Track.GetHeight() : Track.GetWidth();

        if (Run <= 0.0f)
        {
            return Changed;
        }

        const Real32 Share = Content > View && Content > 0.0f ? View / Content : 1.0f;
        const Real32 Size  = Min(Max(Run * Share, kShortest), Run);
        const Real32 Start = Remap(GetOffset(Target, Vertical), 0.0f, Limit, 0.0f, Run - Size);

        return Fit(Bar, Start, Size) || Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scrollbar::Prepare(Element Bar, Element Target)
    {
        const ConstPtr<Rail> Own = Bar.Get<Rail>();

        if (!Own || !Target.IsAlive())
        {
            return;
        }

        // An element made to scroll for the bar has the bar to show where it stands, so it paints no thumb of its own.
        if (!Target.Has<Scroll>())
        {
            Ref<Scroll> Roll = Target.Add<Scroll>();
            Roll.Thumb  = Color::Transparent();
            Roll.Gutter = 0.0f;
        }

        if (!Own->Vertical && !Target.Get<Scroll>()->Horizontal)
        {
            Target.Edit<Scroll>()->Horizontal = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Scrollbar::Fit(Element Bar, Real32 Start, Real32 Size)
    {
        Ref<Tree>       Owner    = Bar.GetTree();
        const Element   Thumb    = GetThumb(Bar);
        const Bool      Vertical = Bar.Get<Rail>()->Vertical;
        ConstRef<Edges> Padding  = Owner.GetBox(Bar.GetHandle()).Padding;
        ConstRef<Box>   Now      = Owner.GetBox(Thumb.GetHandle());

        // Absolute children are placed against the bar's box, so the track's padding is added back here.
        const Real32 Lead   = (Vertical ? Padding.Top : Padding.Left) + Start;
        const Real32 Before = Vertical ? Padding.Left : Padding.Top;
        const Real32 After  = Vertical ? Padding.Right : Padding.Bottom;

        const Bool Same = Vertical
            ? Near(Now.Top, Lead) && Near(Now.Height, Size) && Near(Now.Left, Before) && Near(Now.Right, After)
            : Near(Now.Left, Lead) && Near(Now.Width, Size) && Near(Now.Top, Before) && Near(Now.Bottom, After);

        if (Same)
        {
            return false;
        }

        Ref<Box> Shape = Thumb.EditBox();

        if (Vertical)
        {
            Shape.Top    = Length::Pixels(Lead);
            Shape.Height = Length::Pixels(Size);
            Shape.Left   = Length::Pixels(Before);
            Shape.Right  = Length::Pixels(After);
        }
        else
        {
            Shape.Left   = Length::Pixels(Lead);
            Shape.Width  = Length::Pixels(Size);
            Shape.Top    = Length::Pixels(Before);
            Shape.Bottom = Length::Pixels(After);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Scrollbar::GetTrack(Element Bar)
    {
        Ref<Tree> Owner = Bar.GetTree();

        if (Owner.IsPending(Bar.GetHandle()))
        {
            return Rect::Zero();
        }
        return Owner.GetBox(Bar.GetHandle()).Padding.Inset(Bar.GetFrame());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Scrollbar::Near(ConstRef<Length> Value, Real32 Pixels)
    {
        return Value.Measure == Length::Unit::Pixel && Abs(Value.Value - Pixels) < 0.25f;
    }
}