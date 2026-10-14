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
    /// \brief Represents an element by its slot and that slot's generation, so a stale handle names nothing.
    class Handle final
    {
    public:

        /// \brief Constructs a handle naming nothing.
        ZY_INLINE constexpr Handle()
            : mValue { 0 }
        {
        }

        /// \brief Constructs a handle from the number \ref GetValue hands out.
        ///
        /// \param Value The number, or zero for none.
        ZY_INLINE constexpr explicit Handle(UInt64 Value)
            : mValue { Value }
        {
        }

        /// \brief Constructs a handle naming a slot at a generation.
        ///
        /// \param Slot       The slot, counted from zero.
        /// \param Generation The generation of the slot.
        ZY_INLINE constexpr Handle(UInt32 Slot, UInt32 Generation)
            : mValue { (static_cast<UInt64>(Generation) << 32) | (static_cast<UInt64>(Slot) + 1) }
        {
        }

        /// \brief Checks whether the handle names anything at all, alive or not.
        ///
        /// \return `true` if the handle names a slot, `false` otherwise.
        ZY_INLINE constexpr Bool IsValid() const
        {
            return mValue != 0;
        }

        /// \brief Gets the slot the handle names.
        ///
        /// \return The slot, counted from zero, which is meaningless for a handle naming nothing.
        ZY_INLINE constexpr UInt32 GetSlot() const
        {
            return static_cast<UInt32>(mValue) - 1;
        }

        /// \brief Gets the generation of the slot the handle was handed out at.
        ///
        /// \return The generation, which the slot raises each time its element is destroyed.
        ZY_INLINE constexpr UInt32 GetGeneration() const
        {
            return static_cast<UInt32>(mValue >> 32);
        }

        /// \brief Gets the number the handle is kept as.
        ///
        /// \return The number, zero for a handle naming nothing.
        ZY_INLINE constexpr UInt64 GetValue() const
        {
            return mValue;
        }

        /// \brief Checks whether two handles name the same slot at the same generation.
        ///
        /// \param Other The handle to compare against.
        /// \return `true` if they do, `false` otherwise.
        ZY_INLINE constexpr Bool operator==(ConstRef<Handle> Other) const
        {
            return mValue == Other.mValue;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt64 mValue;
    };

    /// \brief Gets the identifier a name is found by, the one place a name is hashed.
    ///
    /// \param Name The name an element, a style or a signal is known by.
    /// \return The identifier, the same on every build, so it can be a compile-time constant.
    ZY_INLINE constexpr UInt64 Identify(Text Name)
    {
        return Hash(Name);
    }
}