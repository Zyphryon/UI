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

#include "Datum.hpp"
#include "Link.hpp"
#include "Model.hpp"
#include "Path.hpp"
#include "Piece.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Tree.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    class Lexicon;

    /// \brief Provides the binding of elements to the model their tree points at.
    class Binder final
    {
    public:

        /// The room a value written into a text is given up front, in bytes, enough for any number.
        static constexpr UInt kRoom = 24;

        /// \brief Defines the code finding the record an element's facet keeps, or `nullptr` when it holds none.
        using Scoped = Delegate<Ptr<Path>(Element Target)>;

    public:

        /// \brief Makes the record a kind of facet keeps follow its row to another record, as bindings do.
        ///
        /// \param Access The code finding the record an element's facet keeps.
        template<IsFacet Type>
        static void AddScoped(Scoped Access)
        {
            GetHolders().Assign(Facets::Index<Type>(), Move(Access));
        }

        /// \brief Shows a tree's bound values from a model, from the next update on.
        ///
        /// \param Owner  The tree whose bound elements show the model.
        /// \param Source The model, or `nullptr` for none, every bound value then reading as none.
        static void SetModel(Ref<Tree> Owner, Ptr<Model> Source);

        /// \brief Gets the model a tree's bound elements show.
        ///
        /// \param Owner The tree whose model is read.
        /// \return The model, or `nullptr` for none.
        static Ptr<Model> GetModel(ConstRef<Tree> Owner);

        /// \brief Reads a tree's text written `@Key` through string tables, from the next update on.
        ///
        /// \param Owner  The tree whose text is read through the lexicon.
        /// \param Source The lexicon, or `nullptr` for none, every such text then showing its key.
        static void SetLexicon(Ref<Tree> Owner, Ptr<Lexicon> Source);

        /// \brief Gets the string tables a tree's text written `@Key` is read through.
        ///
        /// \param Owner The tree whose lexicon is read.
        /// \return The lexicon, or `nullptr` for none.
        static Ptr<Lexicon> GetLexicon(ConstRef<Tree> Owner);

        /// \brief Shows changed values and texts on what binds them at every update of an interface.
        ///
        /// \param Target The interface whose bindings are kept up to date.
        static void Declare(Ref<Interface> Target);

        /// \brief Binds a field of an element, showing its value at once.
        ///
        /// \param Target The element whose field is bound.
        /// \param Source The text naming the values, as `Gold: {Player.Gold:0}`, or a lexicon key, as `@Menu.Gold`.
        /// \param Write  The code showing the value on the element.
        /// \param Read   The code reading the element's value back, used only for one value alone with no format.
        /// \param Scope  The path of the record relative paths are read from, as `Party[2]`, or empty for the root.
        /// \return `true` if the text binds, `false` otherwise.
        static Bool Attach(Element Target, Text Source, Writer Write, Reader Read = Reader(), Text Scope = Text());

        /// \brief Unbinds every field of an element.
        ///
        /// \param Target The element whose bindings are dropped, which may hold none.
        static void Detach(Element Target);

        /// \brief Checks whether a text names a value in braces, and so binds.
        ///
        /// \param Source The text looked through, where a doubled brace names nothing.
        /// \return `true` if it does, `false` otherwise.
        static Bool IsBound(Text Source);

        /// \brief Checks whether a text is a lexicon key written `@Key`, or an at sign written `@@`.
        ///
        /// \param Source The text looked at.
        /// \return `true` if it is, `false` otherwise.
        static Bool IsLocalized(Text Source);

        /// \brief Shows every bound value a change was stamped on since it was last shown.
        ///
        /// \param Owner The tree whose bindings are shown from its model.
        /// \param Force The flag, `true` to show every value whatever its stamp, `false` to show only changed ones.
        static void Refresh(Ref<Tree> Owner, Bool Force);

        /// \brief Reads every field written `@Key` again from the tree's lexicon, and shows it.
        ///
        /// \param Owner The tree whose lexicon and model are read.
        static void Translate(Ref<Tree> Owner);

        /// \brief Gives an element the bindings of another in place of its own, showing them at once.
        ///
        /// \param Source The element whose bindings are copied, which may hold none.
        /// \param Target The element given them.
        static void Mirror(Element Source, Element Target);

        /// \brief Writes back to the model every field of an element bound both ways, as when its value changed.
        ///
        /// \param Target The element whose value is written back.
        static void Store(Element Target);

        /// \brief Points every binding of an element and of all it holds from one record at another.
        ///
        /// \param Target The element at the top of what is moved, as a row of a list.
        /// \param From   The path of the record the bindings read from, its last step naming the record.
        /// \param To     The path of the record they read from now, which may take more or fewer steps.
        static void Rescope(Element Target, ConstRef<Path> From, ConstRef<Path> To);

        /// \brief Works out the steps a scope gives every path read in it.
        ///
        /// \param Scope  The path of the record, as written, or empty for the root.
        /// \param Output The receiver of the steps, none for the root.
        static void Locate(Text Scope, Ref<Path> Output);

        /// \brief Creates a writer handing a value to a setter taking a flag, a number or a colour.
        ///
        /// \return The writer, which reads the value as the setter's type.
        template<typename Type, void (* Setter)(Element, Type)>
        ZY_INLINE static Writer Put()
        {
            return Writer::Create<&Binder::OnPut<Type, Setter>>();
        }

        /// \brief Creates a writer handing a value, written as text, to a setter taking text.
        ///
        /// \return The writer, which hands text on as it is held and writes any other value out first.
        template<void (* Setter)(Element, Text)>
        ZY_INLINE static Writer PutText()
        {
            return Writer::Create<&Binder::OnPutText<Setter>>();
        }

        /// \brief Creates a writer turning a state of an element on or off by a value read as a flag.
        ///
        /// \return The writer, which sets the state when the value reads as set.
        template<State Bit>
        ZY_INLINE static Writer PutState()
        {
            return Writer::Create<&Binder::OnPutState<Bit>>();
        }

        /// \brief Creates a writer turning a declared state of an element on or off by a value read as a flag.
        ///
        /// \return The writer, which sets the state when the value reads as set.
        template<State (* Bit)()>
        ZY_INLINE static Writer PutState()
        {
            return Writer::Create<&Binder::OnPutState<Bit>>();
        }

        /// \brief Creates a reader taking a value from a getter.
        ///
        /// \return The reader, which holds what the getter gives as a value.
        template<typename Type, Type (* Getter)(Element)>
        ZY_INLINE static Reader Take()
        {
            return Reader::Create<&Binder::OnTake<Type, Getter>>();
        }

        /// \brief Creates a reader taking a state of an element as a flag.
        ///
        /// \return The reader, which gives `true` while the element is in the state.
        template<State Bit>
        ZY_INLINE static Reader TakeState()
        {
            return Reader::Create<&Binder::OnTakeState<Bit>>();
        }

        /// \brief Creates a reader taking a declared state of an element as a flag.
        ///
        /// \return The reader, which gives `true` while the element is in the state.
        template<State (* Bit)()>
        ZY_INLINE static Reader TakeState()
        {
            return Reader::Create<&Binder::OnTakeState<Bit>>();
        }

    private:

        /// \brief Holds where a tree's bound values and texts come from, and what of them was shown last.
        struct Feed final
        {
            /// The model the bound values are read from.
            Ptr<Model>   Source   = nullptr;

            /// The string tables texts written `@Key` are read through.
            Ptr<Lexicon> Tables   = nullptr;

            /// The model last shown.
            Ptr<Model>   Watched  = nullptr;

            /// The revision of the model last shown.
            UInt32       Revision = 0;

            /// The string tables last read.
            Ptr<Lexicon> Heard    = nullptr;

            /// The version of the string tables last read.
            UInt32       Spoken   = 0;

            /// `true` once the interface runs the binder's refresh and listener.
            Bool         Declared = false;
        };

        /// \brief Gets the readers of the records facets keep, made the first time they are asked for.
        ///
        /// \return The readers, by the index of the kind of facet they read.
        static Ref<Table<UInt32, Scoped>> GetHolders();

        /// \brief Gets where a tree's bound values come from, keeping it on the tree the first time.
        ///
        /// \param Owner The tree the feed is kept on.
        /// \return The feed of the tree.
        static Ref<Feed> Supply(Ref<Tree> Owner);

        /// \brief Reads the texts again if the lexicon changed, then shows the model's changed values, as a stage.
        ///
        /// \param Owner The interface whose bindings are shown.
        static void OnRefresh(Ref<Interface> Owner, Real32);

        /// \brief Stores a widget's new value into the model before any handler reads it there, as a listener.
        ///
        /// \param Owner   The interface the event is heard on.
        /// \param Message The event, whose target is written back on \ref Signal::kChange.
        /// \return `false` always, so every handler still hears the event.
        static Bool OnChange(Ref<Interface> Owner, ConstRef<Event> Message, Handle);

        /// \brief Hands a value to a setter taking a flag, a number or a colour.
        ///
        /// \param Target The element the value is shown on.
        /// \param Value  The value, read as the setter's type.
        template<typename Type, void (* Setter)(Element, Type)>
        static void OnPut(Element Target, ConstRef<Datum> Value)
        {
            Setter(Target, Value.As<Type>());
        }

        /// \brief Hands a value, written as text, to a setter taking text.
        ///
        /// \param Target The element the value is shown on.
        /// \param Value  The value, written out unless it already holds text.
        template<void (* Setter)(Element, Text)>
        static void OnPutText(Element Target, ConstRef<Datum> Value)
        {
            // Text is handed on as it is held; only another form is written out first.
            if (Value.GetForm() == Form::Text)
            {
                Setter(Target, Value.GetText());
                return;
            }

            const Str Words = Value.Print();
            Setter(Target, Words);
        }

        /// \brief Turns a state of an element on or off by a value read as a flag.
        ///
        /// \param Target The element whose state is set.
        /// \param Value  The value, read as a flag.
        template<State Bit>
        static void OnPutState(Element Target, ConstRef<Datum> Value)
        {
            Target.SetState(Bit, Value.GetBool());
        }

        /// \brief Turns a declared state of an element on or off by a value read as a flag.
        ///
        /// \param Target The element whose state is set.
        /// \param Value  The value, read as a flag.
        template<State (* Bit)()>
        static void OnPutState(Element Target, ConstRef<Datum> Value)
        {
            Target.SetState(Bit(), Value.GetBool());
        }

        /// \brief Takes a value from a getter.
        ///
        /// \param Target The element read.
        /// \return The value the getter gives.
        template<typename Type, Type (* Getter)(Element)>
        static Datum OnTake(Element Target)
        {
            return Datum(Getter(Target));
        }

        /// \brief Takes a state of an element as a flag.
        ///
        /// \param Target The element read.
        /// \return The flag, set while the element is in the state.
        template<State Bit>
        static Datum OnTakeState(Element Target)
        {
            return Datum(Target.HasState(Bit));
        }

        /// \brief Takes a declared state of an element as a flag.
        ///
        /// \param Target The element read.
        /// \return The flag, set while the element is in the state.
        template<State (* Bit)()>
        static Datum OnTakeState(Element Target)
        {
            return Datum(Target.HasState(Bit()));
        }

        /// \brief Calls an action on every binding of a tree, by element and index.
        ///
        /// \param Owner  The tree whose bindings are walked.
        /// \param Action The code called with each bound element and the index of each of its bindings.
        template<typename Callable>
        static void Sweep(Ref<Tree> Owner, AnyRef<Callable> Action);

        /// \brief Gets the count of bindings an element holds, found again each time it is asked.
        ///
        /// \param Target The element whose bindings are counted.
        /// \return The count, nought when the element is dead or holds none.
        static UInt CountLinks(Element Target);

        /// \brief Reads the text of a binding into its pieces.
        ///
        /// \param Source The text of the binding, as `Gold: {Player.Gold:0}`.
        /// \param Scope  The path relative paths are read from, or empty for the root.
        /// \param Plain  The flag, `true` to take a text naming no value as plain text, `false` to reject it.
        /// \param Output The receiver of the pieces.
        /// \return `true` if every value is named well and, unless \p Plain, at least one is, `false` otherwise.
        static Bool Parse(Text Source, Text Scope, Bool Plain, Ref<Sequence<Piece>> Output);

        /// \brief Reads a lexicon text into its pieces, writing one that names a value badly out as it stands.
        ///
        /// \param Source The text the lexicon gives.
        /// \param Scope  The path relative paths are read from, or empty for the root.
        /// \param Output The receiver of the pieces.
        static void Speak(Text Source, Text Scope, Ref<Sequence<Piece>> Output);

        /// \brief Gets the text the lexicon of an element's tree gives a key.
        ///
        /// \param Target The element whose tree's lexicon is read.
        /// \param Key    The key, as `Menu.Gold`.
        /// \return The text, or the key itself when the tree has no lexicon or the lexicon no such key.
        static Text Lookup(Element Target, Text Key);

        /// \brief Points the bindings of an element and of all it holds at another record, without popups.
        ///
        /// \param Target The element at the top of what is moved.
        /// \param From   The path of the record the bindings read from, its last step naming the record.
        /// \param To     The path of the record they read from now, which may take more or fewer steps.
        static void Descend(Element Target, ConstRef<Path> From, ConstRef<Path> To);

        /// \brief Reads one binding of an element written `@Key` again from the lexicon, and shows it.
        ///
        /// \param Target The element holding the binding.
        /// \param Index  The index of the binding, which a field written out in place leaves alone.
        static void Restate(Element Target, UInt Index);

        /// \brief Reads what one pair of braces holds, an optional `!`, a path and an optional `:` and digits.
        ///
        /// \param Source The text between the braces.
        /// \param Scope  The path relative paths are read from, or empty for the root.
        /// \param Output The receiver of the piece.
        /// \return `true` if it names a value, `false` otherwise.
        static Bool ParseHole(Text Source, Text Scope, Ref<Piece> Output);

        /// \brief Checks whether a binding names one value alone, with no format, so the widget takes it whole.
        ///
        /// \param Entry The binding looked at.
        /// \return `true` if it does, `false` otherwise.
        static Bool IsWhole(ConstRef<Link> Entry);

        /// \brief Gets the newest stamp of the values a binding names.
        ///
        /// \param Data  The model, or `nullptr` for none.
        /// \param Entry The binding whose values are looked up.
        /// \return The stamp, nought when none of them was ever set.
        static UInt32 Stamp(ConstPtr<Model> Data, ConstRef<Link> Entry);

        /// \brief Works out the value a binding shows, the value it names alone or its text with every value in place.
        ///
        /// \param Data  The model, or `nullptr` for none, every value then reading as none.
        /// \param Entry The binding worked out.
        /// \return The value the binding's writer is handed.
        static Datum Evaluate(ConstPtr<Model> Data, ConstRef<Link> Entry);

        /// \brief Shows one binding of an element when a value it names changed since it was last shown.
        ///
        /// \param Target The element holding the binding.
        /// \param Index  The index of the binding.
        /// \param Force  The flag, `true` to show it whatever its stamp, `false` to show it only when changed.
        static void Show(Element Target, UInt Index, Bool Force);

        /// \brief Writes back one binding of an element to the model, when it goes both ways.
        ///
        /// \param Target The element holding the binding.
        /// \param Index  The index of the binding.
        /// \param Data   The model written to.
        static void Keep(Element Target, UInt Index, Ref<Model> Data);

        /// \brief Points the bindings of one element that read from a record at another record.
        ///
        /// \param Target The element whose bindings, and the records its facets keep, are moved.
        /// \param From   The path of the record the bindings read from.
        /// \param To     The path of the record they read from now.
        static void Retarget(Element Target, ConstRef<Path> From, ConstRef<Path> To);

        /// \brief Moves a piece from one record to another when the steps it took from its scope reach the first.
        ///
        /// \param Part The piece moved, whose steps from its scope follow the record's.
        /// \param From The path of the record it may read from.
        /// \param To   The path of the record it reads from now.
        /// \return `true` when it moved, `false` when it reads from elsewhere.
        static Bool Repoint(Ref<Piece> Part, ConstRef<Path> From, ConstRef<Path> To);

        /// \brief Moves a path from one record to another when the steps it took from its scope reach the first.
        ///
        /// \param Where  The path moved.
        /// \param Scoped The steps of the path that came from its scope.
        /// \param From   The path of the record it may read from.
        /// \param To     The path of the record it reads from now.
        /// \return `true` when it moved, `false` when it reads from elsewhere.
        static Bool Reroute(Ref<Path> Where, UInt Scoped, ConstRef<Path> From, ConstRef<Path> To);

        /// \brief Checks whether a pane was opened for an element or anything it holds, as a row's menu or its submenu.
        ///
        /// \param Owner  The tree holding both.
        /// \param Root   The root of the pane.
        /// \param Target The element the pane may have been opened for.
        /// \return `true` if it was, `false` otherwise.
        static Bool IsOpenedFor(ConstRef<Tree> Owner, Handle Root, Handle Target);

        /// \brief Gathers the roots of the panes opened for an element or for anything it holds.
        ///
        /// \param Owner  The tree holding them.
        /// \param Target The element the panes were opened for.
        /// \param Output The receiver of the roots.
        static void Gather(ConstRef<Tree> Owner, Handle Target, Ref<Sequence<Handle>> Output);

        /// \brief Adds the text kept so far as a piece of its own, when there is any.
        ///
        /// \param Literal The text kept so far, emptied.
        /// \param Output  The pieces it is added to.
        static void Flush(Ref<Str> Literal, Ref<Sequence<Piece>> Output);
    };
}