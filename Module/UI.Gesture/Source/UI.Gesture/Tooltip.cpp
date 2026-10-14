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

#include "Tooltip.hpp"
#include "Drag.hpp"
#include "Facet/Hint.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Visual/Caption.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/System/Dispatcher.hpp"
#include "UI.Core/System/Pointer.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::SetText(Element Target, Text Content)
    {
        Ref<Hint> Found = Target.Obtain<Hint>();
        Found.Content = Content;
        Found.Fill    = Filler::Create<&Tooltip::Write>();
        Found.Reader  = nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::SetTemplate(Element Target, Filler Fill)
    {
        Ref<Hint> Found = Target.Obtain<Hint>();
        Found.Fill   = Move(Fill);
        Found.Reader = nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::SetTemplate(Element Target, Ref<Markup> Reader, Text Template)
    {
        Ref<Hint> Found = Target.Obtain<Hint>();
        Found.Fill     = Filler::Create<&Tooltip::Spawn>();
        Found.Reader   = AddressOf(Reader);
        Found.Template = Template;

        // Named in a row of a list, it is made later reading the row's record.
        Found.Scope = Reader.GetScope();
        Binder::Locate(Found.Scope, Found.Within);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::Clear(Element Target)
    {
        if (Target.Has<Hint>())
        {
            Target.Remove<Hint>();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text Tooltip::GetText(Element Target)
    {
        const ConstPtr<Hint> Found = Target.Get<Hint>();
        return Found ? Text(Found->Content) : Text();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::Read(Ref<Markup> Reader, Element Target, ConstRef<JsonValue> Node)
    {
        const ConstPtr<JsonValue> Field = Markup::GetField(Node, "Tooltip");

        if (!Field)
        {
            return;
        }

        if (!Field->IsString())
        {
            LOG_W("A node's 'Tooltip' must be text or the name of a template, so it shows none");
            return;
        }

        // A name the markup knows is a template; anything else is the text shown, which may name values.
        const Text Content = Field->GetString();

        if (!Binder::IsBound(Content) && Reader.HasTemplate(Content))
        {
            SetTemplate(Target, Reader, Content);
        }
        else
        {
            SetText(Target, Content);
            Reader.Bind(Target, Node, "Tooltip", Binder::PutText<&Tooltip::SetText>());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<Path> Tooltip::GetWithin(Element Target)
    {
        const Ptr<Hint> Tip = Target.Get<Hint>();
        return Tip ? AddressOf(Tip->Within) : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::Declare(Ref<Interface> Target)
    {
        if (Target.Find<Session>())
        {
            return;
        }

        Target.Provide<Session>();

        Pointer::Hook Gesture;
        Gesture.Pressed = Pointer::Hook::OnPressed::Create<&Tooltip::OnPressed>();
        Gesture.Turned  = Pointer::Hook::OnTurned::Create<&Tooltip::OnTurned>();
        Gesture.Picked  = Pointer::Hook::OnPointer::Create<&Tooltip::OnPicked>();
        Target.GetCursor().AddHook(Move(Gesture));

        // A tooltip due is made before anything is dressed, so it shows at once.
        Target.AddStage(Interface::Phase::Prepare, 41, Interface::Stage::Create<&Tooltip::OnLinger>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Tooltip::GetPane(Ref<Interface> Owner)
    {
        const ConstPtr<Session> Run = Owner.Find<Session>();
        return Run ? Run->Pane : Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::SetDelay(Ref<Interface> Owner, Real32 Seconds)
    {
        if (const Ptr<Session> Run = Owner.Find<Session>())
        {
            Run->Delay = Seconds;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::Silence(Ref<Interface> Owner)
    {
        if (const Ptr<Session> Run = Owner.Find<Session>())
        {
            Hush(Owner, * Run);
            Run->Hushed = Run->Hinted;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::Write(Element Holder, Element Source)
    {
        const Element Line = Holder.Create();
        Line.Add<Caption>().Content = GetText(Source);
        Line.SetStyle("Tooltip.Label");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::Spawn(Element Holder, Element Source)
    {
        const ConstPtr<Hint> Found = Source.Get<Hint>();

        if (!Found || !Found->Reader)
        {
            return;
        }

        // Copied, since the template may give tooltips too and move the store under this one.
        const Str  Template(Found->Template);
        const Str  Scope(Found->Scope);
        const Path Within = Found->Within;

        if (!Found->Reader->Spawn(Holder, Template, Scope, Within).IsAlive())
        {
            LOG_W("The tooltip template '{0}' makes nothing, so the tooltip shows empty", Text(Template));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::Refresh(Ref<Interface> Owner)
    {
        const Ptr<Session> Run = Owner.Find<Session>();

        if (Run && Owner.IsAlive(Run->Pane) && Owner.Has<Hint>(Run->Shown) && !Owner.IsPending(Run->Shown))
        {
            Fill(Owner, * Run, Run->Shown);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::OnPressed(Ref<Interface> Owner, ZyInput::Button)
    {
        Silence(Owner);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::OnTurned(Ref<Interface> Owner, Bool Stopped)
    {
        // A turn a control took changed only what it shows, as a thumbnail's face, so its tooltip stays to show it.
        if (!Stopped)
        {
            Silence(Owner);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::OnPicked(Ref<Interface> Owner)
    {
        Ref<Session> Run = * Owner.Find<Session>();

        // A tooltip belongs to what the pointer rests on; moving on to something else starts its wait over.
        if (const Handle Source = Hinted(Owner, Owner.GetOver()); Source != Run.Hinted)
        {
            Hush(Owner, Run);
            Run.Hinted = Source;
            Run.Hushed = Handle();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::OnLinger(Ref<Interface> Owner, Real32 Delta)
    {
        Ref<Session>    Run   = * Owner.Find<Session>();
        Ref<Dispatcher> Route = Owner.GetDispatcher();

        // A tooltip whose element went away, was hidden with its pane, or lost its tooltip, goes with it.
        const Bool Valid = Run.Shown.IsValid();

        if (Valid && (!Owner.Has<Hint>(Run.Shown) || Owner.IsPending(Run.Shown) || !Route.IsShown(Run.Shown)))
        {
            Hush(Owner, Run);
        }

        const Bool Waiting = Owner.Has<Hint>(Run.Hinted) && !Owner.IsPending(Run.Hinted) && Route.IsShown(Run.Hinted)
            && Run.Hinted != Run.Hushed && Run.Hinted != Run.Shown && !Drag::GetDragged(Owner).IsValid()
            && !Owner.GetCursor().IsHeld();

        if (!Waiting)
        {
            return;
        }

        Run.Clock += Delta;

        if (Run.Clock >= Run.Delay)
        {
            Tip(Owner, Run);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Tooltip::Hinted(Ref<Interface> Owner, Handle Element)
    {
        for (Handle Current = Element; Current.IsValid(); Current = Owner.GetParent(Current))
        {
            if (Owner.Has<Hint>(Current))
            {
                return Current;
            }
        }
        return Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::Fill(Ref<Interface> Owner, Ref<Session> Run, Handle Source)
    {
        Sequence<Handle> Shown;
        Owner.Children(Run.Pane, [&Shown](Handle Child)
        {
            Shown.Append(Child);
        });

        for (const Handle Child : Shown)
        {
            Owner.Destroy(Child);
        }

        // Copied, since the code may add a facet of the kind and move the store under it.
        const Filler  Code = Owner.Get<Hint>(Source)->Fill;
        const Element Holder(Owner, Run.Pane);
        const Element Explained(Owner, Source);

        if (Code.IsEmpty())
        {
            Write(Holder, Explained);
        }
        else
        {
            Code(Holder, Explained);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::Tip(Ref<Interface> Owner, Ref<Session> Run)
    {
        // One pane serves every tooltip, made the first time one shows and dressed by the theme as `Tooltip`.
        if (!Owner.IsAlive(Run.Pane))
        {
            Run.Pane = Owner.Open(kLayer);
            Owner.SetStyle(Run.Pane, Identify("Tooltip"));
            Owner.SetState(Run.Pane, State::Hidden, true);
            Owner.EditBox(Run.Pane).Placement = Position::Absolute;
            Owner.Confine(Run.Pane, kGap);
        }

        // What it showed for another element goes, even while it still fades out.
        Fill(Owner, Run, Run.Hinted);

        const Vector2 Corner = Owner.GetPointer() + Vector2(kGap * 0.5f, kGap);
        Ref<Box>      Shape  = Owner.EditBox(Run.Pane);
        Shape.Left = Length::Pixels(Corner.GetX());
        Shape.Top  = Length::Pixels(Corner.GetY());

        Owner.SetState(Run.Pane, State::Hidden, false);
        Owner.Raise(Run.Pane);
        Run.Shown = Run.Hinted;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tooltip::Hush(Ref<Interface> Owner, Ref<Session> Run)
    {
        // Hidden rather than emptied, so it plays its way out showing what it showed.
        if (Run.Shown.IsValid() && Owner.IsAlive(Run.Pane))
        {
            Owner.SetState(Run.Pane, State::Hidden, true);
        }

        Run.Shown = Handle();
        Run.Clock = 0.0f;
    }
}