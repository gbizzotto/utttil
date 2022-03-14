
#pragma once

namespace utttil {

template<typename T>
struct no_init
{
    T value;
    no_init() noexcept {
        // do nothing
        static_assert(sizeof *this == sizeof value, "invalid size");
        static_assert(__alignof *this == __alignof value, "invalid alignment");
    }
    template<typename U>
    no_init(U u)
        : value(u)
    {}
    operator T() const { return value; }
    T & operator=(T other)
    {
    	value = other;
    	return value;
    }
    T & operator+=(T other)
    {
    	value += other;
    	return value;
    }
};

} // namespace
