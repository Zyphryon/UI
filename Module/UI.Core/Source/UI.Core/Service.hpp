// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Defines the interface a module's state, kept by an interface, is destroyed through.
    class Service
    {
    public:

        /// \brief Destroys the state.
        virtual ~Service() = default;
    };

    /// \brief Represents a module's state of one type, kept by an interface.
    template<typename Type>
    class Kept final : public Service
    {
    public:

        /// \brief Constructs the state from its arguments.
        ///
        /// \param Parameters The arguments the state is built from.
        template<typename... Arguments>
        explicit Kept(AnyRef<Arguments>... Parameters)
            : mValue { Forward<Arguments>(Parameters)... }
        {
        }

        /// \brief Gets the state, to change it.
        ///
        /// \return The state, which lives as long as this holder.
        ZY_INLINE Ref<Type> Get()
        {
            return mValue;
        }

        /// \brief Gets the state.
        ///
        /// \return The state, which lives as long as this holder.
        ZY_INLINE ConstRef<Type> Get() const
        {
            return mValue;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Type mValue;
    };

    /// \brief Provides the index each type of service is known by, handed out the first time the type is named.
    class ZY_API Services final
    {
    public:

        /// \brief Gets the index of a type of service, handing one out the first time it is asked for.
        ///
        /// \return The index, the same for the life of the process.
        template<typename Type>
        static UInt32 Index()
        {
            static UInt32 kIndex = kUnset;

            if (kIndex == kUnset)
            {
                kIndex = Declare(Hash<Type>());
            }
            return kIndex;
        }

    private:

        /// The index a type of service holds until it is first asked for.
        static constexpr UInt32 kUnset = kMaximum<UInt32>;

        /// \brief Hands out the index of a type, the next one unless another library already named it.
        ///
        /// \param Key The hash of the type's name, the same in every library.
        /// \return The index the type of service is known by from now on.
        static UInt32 Declare(UInt64 Key);
    };
}