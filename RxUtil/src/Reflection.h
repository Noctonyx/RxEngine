////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde (shane@noctonyx.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

//
// Created by shane on 9/06/2021.
//

#ifndef RXUTIL_REFLECTION_H
#define RXUTIL_REFLECTION_H

template<
    typename Callable,
    typename Tuple,
    size_t... Indexes
>
constexpr void tuple_for_each(Tuple && t, Callable && f, std::index_sequence<Indexes...>)
{
    (f(std::get<Indexes>(t)), ...);
}

template<
    typename Callable,
    typename... Args,
    template<typename...> typename Tuple,
    typename Is = std::make_index_sequence<sizeof...(Args)>
>
constexpr void tuple_for_each(Tuple<Args...> && t, Callable && f)
{
    tuple_for_each(t, f, Is{});
}

template<typename T>
struct ReflectionLayout
{
    static constexpr auto GetLayout()
    {
        return std::make_tuple(
            std::make_tuple(
                "#Unknown", "Unknown"
            )
        );
    }
};

template<typename T>
struct Reflection
{
    static constexpr bool has_reflection_info = false;
    static constexpr char const * name{"Unknown"};

    static constexpr auto layout()
    { return ReflectionLayout<T>::GetLayout(); }

    static constexpr bool is_complex{false};
};

#define REGISTER_REFLECTION_INFO(Class) \
template<> \
struct Reflection<Class> \
{ \
static constexpr bool has_reflection_info = true; \
static constexpr char const* name{ #Class }; \
static constexpr auto layout() { return ReflectionLayout<Class>::GetLayout(); } \
static constexpr bool is_complex{ true }; \
};

#define REGISTER_REFLECTION_LAYOUT(Class) \
template<> \
struct ReflectionLayout<Class> \
{ \
    using Type = Class; \
    static constexpr auto GetLayout() \
    { \
        return std::make_tuple(

#define MEMBER(Name) std::make_tuple(#Name, &Type::Name)

#define END_REFLECTION_LAYOUT(Class) \
        ); \
    } \
}; \
REGISTER_REFLECTION_INFO(Class)

#endif //RXUTIL_REFLECTION_H
