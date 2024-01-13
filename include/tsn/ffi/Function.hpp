#pragma once
#include <tsn/ffi/Function.h>

namespace tsn {
    namespace ffi {
        template <typename Ret, typename... Args>
        FunctionPointer::FunctionPointer(Ret (*ptr)(Args...)) : FunctionPointer(reinterpret_cast<void*>(&ptr), sizeof(ptr)) {}

        template <typename Cls, typename Ret, typename... Args>
        FunctionPointer::FunctionPointer(Ret (Cls::*ptr)(Args...)) : FunctionPointer(reinterpret_cast<void*>(&ptr), sizeof(ptr)) {}

        template <typename Cls, typename Ret, typename... Args>
        FunctionPointer::FunctionPointer(Ret (Cls::*ptr)(Args...) const) : FunctionPointer(reinterpret_cast<void*>(&ptr), sizeof(ptr)) {}

        template <typename Ret, typename... Args>
        void FunctionPointer::get(Ret (**ptr)(Args...)) const {
            constexpr u32 sz = sizeof(Ret(*)(Args...));

            for (u32 i = 0;i < sz;i++) ((u8*)ptr)[i] = m_data[i];
        }

        template <typename Cls, typename Ret, typename... Args>
        void FunctionPointer::get(Ret (Cls::**ptr)(Args...)) const {
            constexpr u32 sz = sizeof(Ret(Cls::*)(Args...));

            for (u32 i = 0;i < sz;i++) ((u8*)ptr)[i] = m_data[i];
        }

        template <typename Cls, typename Ret, typename... Args>
        void FunctionPointer::get(Ret (Cls::**ptr)(Args...) const) const {
            constexpr u32 sz = sizeof(Ret(Cls::*)(Args...) const);

            for (u32 i = 0;i < sz;i++) ((u8*)ptr)[i] = m_data[i];
        }
    };
};