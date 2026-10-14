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

#include "Drag.hpp"
#include "Facet/Draggable.hpp"
#include "Tooltip.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Visual/Backdrop.hpp"
#include "UI.Core/Facet/Visual/Bar.hpp"
#include "UI.Core/Facet/Visual/Caption.hpp"
#include "UI.Core/Facet/Visual/Look.hpp"
#include "UI.Core/Facet/Visual/Picture.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/States.hpp"
#include "UI.Core/System/Pointer.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Data/Datum.hpp"
#include "UI.Data/Link.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::SetSource(Element Target, Text Kind, ConstRef<Datum> Payload)
    {
        Ref<Draggable> Source = Target.Obtain<Draggable>();
        Source.Kind    = Identify(Kind);
        Source.Payload = Payload;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::SetPayload(Element Target, ConstRef<Datum> Payload)
    {
        Target.Obtain<Draggable>().Payload = Payload;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Datum Drag::GetPayload(Element Target)
    {
        const ConstPtr<Draggable> Source = Target.Get<Draggable>();
        return Source ? Source->Payload : Datum();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::SetGhost(Element Target, Filler Fill)
    {
        Target.Obtain<Draggable>().Ghost = Move(Fill);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::SetGhost(Element Target, Ref<Markup> Reader, Text Template)
    {
        Ref<Draggable> Source = Target.Obtain<Draggable>();
        Source.Ghost    = Filler::Create<&Drag::Spawn>();
        Source.Reader   = AddressOf(Reader);
        Source.Template = Template;

        // Named in a row of a list, it is made later reading the row's record.
        Source.Scope = Reader.GetScope();
        Binder::Locate(Source.Scope, Source.Within);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::SetTarget(Element Target, Text Kinds)
    {
        Ref<Droppable> Accepts = Target.Obtain<Droppable>();
        Accepts.Kinds.Clear();
        Accept(Accepts, Kinds);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Drag::Accepts(Element Target, Element Source)
    {
        const ConstPtr<Droppable> Accepted = Target.Get<Droppable>();
        const ConstPtr<Draggable> Carried  = Source.Get<Draggable>();

        return Accepted && Carried && (Accepted->Kinds.IsEmpty() || Accepted->Kinds.Contains(Carried->Kind));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Read(Ref<Markup> Reader, Element Target, ConstRef<JsonValue> Node)
    {
        const ConstPtr<JsonValue> Carried  = Markup::GetField(Node, "Drag");
        const ConstPtr<JsonValue> Kind     = Markup::GetField(Node, "DragKind");
        const ConstPtr<JsonValue> Template = Markup::GetField(Node, "DragTemplate");
        const ConstPtr<JsonValue> Dropped  = Markup::GetField(Node, "Drop");

        // Any of the source's fields makes it one; a value naming the model carries what the model holds.
        if (Carried || Kind || (Template && Template->IsString()))
        {
            const Bool Bound = Carried && Carried->IsString() && Binder::IsBound(Carried->GetString());

            SetSource(Target, Markup::GetText(Node, "DragKind"), Carried && !Bound ? Parse(* Carried) : Datum());

            if (Bound)
            {
                Reader.Bind(Target, Node, "Drag", Writer::Create<&Drag::SetPayload>());
            }
        }

        if (Template && Template->IsString())
        {
            SetGhost(Target, Reader, Template->GetString());
        }
        else if (Template)
        {
            LOG_W("A node's 'DragTemplate' must name a template, so its drags carry a copy of it");
        }

        if (!Dropped)
        {
            return;
        }

        // `true` accepts every kind, a text one kind, and a list each kind it names.
        if (Dropped->IsBool() && Dropped->GetBool())
        {
            SetTarget(Target);
        }
        else if (Dropped->IsString())
        {
            SetTarget(Target, Dropped->GetString());
        }
        else if (Dropped->IsArray())
        {
            SetTarget(Target);

            Ref<Droppable> Accepts = * Target.Get<Droppable>();

            for (ConstRef<JsonValue> Entry : Dropped->GetArray())
            {
                if (Entry.IsString())
                {
                    Accept(Accepts, Entry.GetString());
                }
            }
        }
        else if (!Dropped->IsBool())
        {
            LOG_W("A node's 'Drop' must be a kind, a list of kinds or 'true', so nothing is dropped on it");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<Path> Drag::GetWithin(Element Target)
    {
        const Ptr<Draggable> Source = Target.Get<Draggable>();
        return Source ? AddressOf(Source->Within) : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    State Drag::Dragging()
    {
        static const State kBit = States::Declare("Dragging");
        return kBit;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    State Drag::DropHover()
    {
        static const State kBit = States::Declare("DropHover");
        return kBit;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Declare(Ref<Interface> Target)
    {
        if (Target.Find<Session>())
        {
            return;
        }

        Target.Provide<Session>();

        Pointer::Hook Gesture;
        Gesture.Press   = Pointer::Hook::OnButton::Create<&Drag::OnPress>();
        Gesture.Pressed = Pointer::Hook::OnPressed::Create<&Drag::OnPressed>();
        Gesture.Move    = Pointer::Hook::OnMove::Create<&Drag::OnMove>();
        Gesture.Release = Pointer::Hook::OnButton::Create<&Drag::OnRelease>();
        Gesture.Key     = Pointer::Hook::OnKey::Create<&Drag::OnKey>();
        Target.GetCursor().AddHook(Move(Gesture));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Drag::GetDragged(Ref<Interface> Owner)
    {
        const ConstPtr<Session> Run = Owner.Find<Session>();
        return Run ? Run->Dragged : Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Drag::GetTarget(Ref<Interface> Owner)
    {
        const ConstPtr<Session> Run = Owner.Find<Session>();
        return Run ? Run->Target : Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Drag::GetGhost(Ref<Interface> Owner)
    {
        const ConstPtr<Session> Run = Owner.Find<Session>();
        return Run ? Run->Ghost : Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Cancel(Ref<Interface> Owner)
    {
        const Ptr<Session> Run = Owner.Find<Session>();

        if (!Run)
        {
            return;
        }

        if (Run->Dragged.IsValid())
        {
            Land(Owner, * Run, Handle());
        }
        Run->Candidate = Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Spawn(Element Holder, Element Source)
    {
        const ConstPtr<Draggable> Found = Source.Get<Draggable>();

        if (!Found || !Found->Reader)
        {
            return;
        }

        // Copied, since making the ghost may make sources too and move the store under this one.
        const Str  Template(Found->Template);
        const Str  Scope(Found->Scope);
        const Path Within = Found->Within;

        if (!Found->Reader->Spawn(Holder, Template, Scope, Within).IsAlive())
        {
            LOG_W("The ghost template '{0}' makes nothing, so the drag carries an empty ghost", Text(Template));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Drag::OnPress(Ref<Interface> Owner, ZyInput::Button)
    {
        Ref<Session> Run = * Owner.Find<Session>();

        // Another button going down while something is carried lets it go nowhere.
        if (!Run.Dragged.IsValid())
        {
            return false;
        }

        Land(Owner, Run, Handle());
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::OnPressed(Ref<Interface> Owner, ZyInput::Button Button)
    {
        if (Button != ZyInput::Button::Left)
        {
            return;
        }

        Ref<Session> Run = * Owner.Find<Session>();
        Run.Candidate = Carrier(Owner, Owner.GetOver());
        Run.PressAt   = Owner.GetPointer();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Drag::OnMove(Ref<Interface> Owner)
    {
        Ref<Session> Run = * Owner.Find<Session>();

        // What the pointer carries follows it, and only the targets it passes over hear it; one that died ends it.
        if (Run.Dragged.IsValid())
        {
            if (!Owner.IsAlive(Run.Dragged))
            {
                Land(Owner, Run, Handle());
                return false;
            }
            Carry(Owner, Run);
        }
        else if (Owner.IsAlive(Run.Candidate) && Owner.GetPointer().GetDistance(Run.PressAt) >= kThreshold)
        {
            // A press on something the pointer can carry becomes a drag once it moved far enough to be no click.
            Lift(Owner, Run);
        }
        else
        {
            return false;
        }

        Owner.GetCursor().Repick();
        Aim(Owner, Run);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Drag::OnRelease(Ref<Interface> Owner, ZyInput::Button Button)
    {
        if (Button != ZyInput::Button::Left)
        {
            return false;
        }

        Ref<Session> Run = * Owner.Find<Session>();
        Run.Candidate = Handle();

        if (!Run.Dragged.IsValid())
        {
            return false;
        }

        // Letting go of what is carried drops it on the target under the pointer, or nowhere when it died.
        if (Owner.IsAlive(Run.Dragged))
        {
            Aim(Owner, Run);
        }
        Land(Owner, Run, Owner.IsAlive(Run.Dragged) ? Run.Target : Handle());
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Drag::OnKey(Ref<Interface> Owner, ZyInput::Key Key)
    {
        Ref<Session> Run = * Owner.Find<Session>();

        // Escape lets go of what is carried before anything holding the keyboard hears it.
        if (!Run.Dragged.IsValid() || Key != ZyInput::Key::Escape)
        {
            return false;
        }

        Land(Owner, Run, Handle());
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Drag::Carrier(Ref<Interface> Owner, Handle Element)
    {
        for (Handle Current = Element; Current.IsValid(); Current = Owner.GetParent(Current))
        {
            if (Owner.Has<Draggable>(Current))
            {
                return Owner.GetDispatcher().IsUsable(Current) ? Current : Handle();
            }
        }
        return Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Drag::Receiver(Ref<Interface> Owner, ConstRef<Session> Run, Handle Element)
    {
        const ConstPtr<Draggable> Carried = Owner.Get<Draggable>(Run.Dragged);
        const UInt64              Kind    = Carried ? Carried->Kind : 0;

        for (Handle Current = Element; Current.IsValid(); Current = Owner.GetParent(Current))
        {
            const ConstPtr<Droppable> Accepts = Owner.Get<Droppable>(Current);

            if (!Accepts)
            {
                continue;
            }

            // Nothing is dropped on itself, nor on what it holds.
            if (Owner.GetDispatcher().IsWithin(Current, Run.Dragged))
            {
                return Handle();
            }

            const Bool Taken = Accepts->Kinds.IsEmpty() || Accepts->Kinds.Contains(Kind);

            if (Taken && Owner.GetDispatcher().IsUsable(Current))
            {
                return Current;
            }
        }
        return Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Lift(Ref<Interface> Owner, Ref<Session> Run)
    {
        const Handle Source = Run.Candidate;
        Run.Candidate = Handle();

        // A press that became a drag is no click, so the control it went down on lets it go.
        Owner.GetCursor().Unpress();
        Tooltip::Silence(Owner);

        Run.Dragged = Source;
        Run.Hold    = Run.PressAt - Owner.GetFrame(Source).GetPosition();

        Owner.SetState(Source, Dragging(), true);

        // The ghost stands in a pane of its own over everything, where the pointer passes through it.
        Run.Ghost = Owner.Open(kLayer);
        Owner.SetStyle(Run.Ghost, Identify("Ghost"));
        Owner.EditBox(Run.Ghost).Placement = Position::Absolute;
        Carry(Owner, Run);

        // Copied, since the code may add a facet of the kind and move the store under it.
        const ConstPtr<Draggable> Carried = Owner.Get<Draggable>(Source);
        const Filler              Code    = Carried ? Carried->Ghost : Filler();

        if (!Code.IsEmpty())
        {
            Code(Element(Owner, Run.Ghost), Element(Owner, Source));
        }
        else
        {
            Copy(Owner, Source, Run.Ghost);
        }

        Tell(Owner, kStart, Source, Handle());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Carry(Ref<Interface> Owner, ConstRef<Session> Run)
    {
        if (!Owner.IsAlive(Run.Ghost))
        {
            return;
        }

        // The ghost keeps the place the pointer took hold of it by.
        const Vector2 Corner = Owner.GetPointer() - Run.Hold;
        Ref<Box>      Shape  = Owner.EditBox(Run.Ghost);
        Shape.Left = Length::Pixels(Corner.GetX());
        Shape.Top  = Length::Pixels(Corner.GetY());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Aim(Ref<Interface> Owner, Ref<Session> Run)
    {
        const Handle Found = Receiver(Owner, Run, Owner.GetOver());

        if (Found == Run.Target)
        {
            return;
        }

        const Handle Before = Run.Target;
        Run.Target = Found;

        if (Owner.IsAlive(Before))
        {
            Owner.SetState(Before, DropHover(), false);
            Tell(Owner, kLeave, Before, Run.Dragged);
        }

        // A handler of the leave may have ended the drag, or destroyed the target.
        if (Run.Dragged.IsValid() && Owner.IsAlive(Found) && Found == Run.Target)
        {
            Owner.SetState(Found, DropHover(), true);
            Tell(Owner, kEnter, Found, Run.Dragged);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Land(Ref<Interface> Owner, Ref<Session> Run, Handle Target)
    {
        const Handle Source  = Run.Dragged;
        const Handle Ghost   = Run.Ghost;
        const Handle Hovered = Run.Target;

        // The drag is over before anyone hears of it, so a handler may start another.
        Run.Dragged = Handle();
        Run.Ghost   = Handle();
        Run.Target  = Handle();

        if (Owner.IsAlive(Hovered))
        {
            Owner.SetState(Hovered, DropHover(), false);
        }

        if (Owner.IsAlive(Source))
        {
            Owner.SetState(Source, Dragging(), false);
        }

        if (Owner.IsAlive(Ghost))
        {
            Owner.Destroy(Ghost);
        }

        const Bool Dropped = Owner.IsAlive(Target);

        if (Dropped)
        {
            Tell(Owner, kDrop, Target, Source);
        }
        else if (Owner.IsAlive(Hovered))
        {
            // A target lit by a drag let go elsewhere hears it leave, as one the pointer moved off does.
            Tell(Owner, kLeave, Hovered, Source);
        }

        if (Owner.IsAlive(Source))
        {
            Tell(Owner, kEnd, Source, Dropped ? Target : Handle());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Tell(Ref<Interface> Owner, UInt64 Kind, Handle Target, Handle Other)
    {
        Event Message(Kind, Target);
        Message.Other    = Other;
        Message.Position = Owner.GetPointer();
        Owner.GetDispatcher().Dispatch(Message);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Copy(Ref<Interface> Owner, Handle Source, Handle Ghost)
    {
        const Rect Frame = Owner.GetFrame(Source);

        // Walked a level at a time, so every copy is made after its parent and before its younger siblings.
        Sequence<Fold> Queue;
        Queue.Append(Source, Ghost);

        for (UInt Cursor = 0; Cursor < Queue.GetSize() && Cursor < kMaxGhost; ++Cursor)
        {
            const Fold   Next = Queue[Cursor];
            const Handle Made = Owner.Create(Next.Into);

            Imitate(Owner, Next.From, Made);

            for (Handle Child = Owner.GetFirst(Next.From); Child.IsValid(); Child = Owner.GetNext(Child))
            {
                Queue.Append(Child, Made);
            }
        }

        // The copy of the element flows in the ghost at the size it was laid out at, wherever it stood.
        const Handle Top   = Owner.GetFirst(Ghost);
        Ref<Box>     Shape = Owner.EditBox(Top);
        Shape.Placement = Position::Flow;
        Shape.Left      = Length::Auto();
        Shape.Top       = Length::Auto();
        Shape.Right     = Length::Auto();
        Shape.Bottom    = Length::Auto();
        Shape.Margin    = Edges();
        Shape.Width     = Length::Pixels(Frame.GetWidth());
        Shape.Height    = Length::Pixels(Frame.GetHeight());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<IsFacet Type>
    void Drag::Clone(Ref<Interface> Owner, Handle From, Handle Into)
    {
        // Copied out first, since adding the facet may move the store holding the original.
        if (const ConstPtr<Type> Found = Owner.Get<Type>(From))
        {
            const Type Copied = * Found;
            Owner.Add<Type>(Into, Copied);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Imitate(Ref<Interface> Owner, Handle From, Handle Into)
    {
        // The ghost is what is carried, not the source left behind, so it is never drawn as carried or as a target.
        const State Passing = States::kPassing | Dragging() | DropHover();

        Owner.SetStyle(Into, Owner.GetStyle(From));

        const Box Shape = Owner.GetBox(From);
        Owner.EditBox(Into) = Shape;

        // Only the states that say what it is are copied; hover, press and focus belong to the original.
        Owner.SetState(Into, ClearBit(Owner.GetStates(From), Passing), true);

        Clone<Look>(Owner, From, Into);
        Clone<Caption>(Owner, From, Into);
        Clone<Picture>(Owner, From, Into);
        Clone<Bar>(Owner, From, Into);
        Clone<Backdrop>(Owner, From, Into);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Datum Drag::Parse(ConstRef<JsonValue> Value)
    {
        if (Value.IsBool())
        {
            return Datum(Value.GetBool());
        }

        if (Value.IsNumber())
        {
            return Datum(Value.GetNumber<Real64>());
        }
        return Value.IsString() ? Datum(Value.GetString()) : Datum();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Drag::Accept(Ref<Droppable> Accepts, Text Kinds)
    {
        StrSplit(Kinds, ',', [&](Text Token)
        {
            if (const Text Kind = StrTrim(Token); !Kind.IsEmpty())
            {
                Accepts.Kinds.Append(Identify(Kind));
            }
        });
    }
}