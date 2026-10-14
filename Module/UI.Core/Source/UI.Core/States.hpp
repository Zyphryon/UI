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

#include "Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Provides the states a module declares by name beyond the five the interface keeps, one bit each.
    class States final
    {
    public:

        /// The states the interface keeps for itself.
        static constexpr State kCore    = State::Hidden | State::Disabled | State::Hover | State::Press | State::Focus;

        /// The states that end with what caused them, so a copy of an element never takes them.
        static constexpr State kPassing = State::Hover | State::Press | State::Focus;

    public:

        /// \brief Declares a state by name, or finds the one declared before by that name.
        ///
        /// \param Name The name, as a theme writes it.
        /// \return The state, one bit.
        static State Declare(Text Name);

        /// \brief Finds a state by name, one of the interface's own or a declared one, in any case.
        ///
        /// \param Name   The name to look for.
        /// \param Output The receiver of the state.
        /// \return `true` if a state has that name, `false` otherwise.
        static Bool Find(Text Name, Ref<State> Output);

    private:

        /// The first bit a declared state may take.
        static constexpr UInt32 kFirst = 5;

        /// \brief Holds one named state.
        struct Entry final
        {
            /// The name the state was declared by, matched in any case.
            Str   Name;

            /// The bit the name stands for.
            State Bit = State::None;
        };

        /// \brief Holds every state declared so far.
        struct Registry final
        {
            /// The named states, the interface's own first.
            Sequence<Entry> Entries;

            /// The bit the next state takes.
            UInt32          Next = kFirst;
        };

        /// \brief Gets the registry, made with the interface's own states the first time it is asked for.
        ///
        /// \return The registry of every state declared so far.
        static Ref<Registry> GetRegistry();

        /// \brief Creates the registry holding the interface's own states.
        ///
        /// \return The registry naming the five states of \ref kCore.
        static Registry Seed();
    };
}