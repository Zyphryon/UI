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
        /// \param Output Receives the state, left untouched when no state has the name.
        /// \return `true` if a state has that name, `false` otherwise.
        static Bool Find(Text Name, Ref<State> Output);

    private:

        /// \brief Gets the name of every state, made with the interface's own the first time they are asked for.
        ///
        /// \return The names, the one at each place naming the state of that bit.
        static Ref<Sequence<Str>> GetNames();

        /// \brief Creates the names of the states the interface keeps for itself.
        ///
        /// \return The names of the five states of \ref kCore, in the order of their bits.
        static Sequence<Str> Seed();
    };
}