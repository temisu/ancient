/* Copyright (C) Teemu Suutari */

#ifndef OVERFLOW_CHECK_HPP
#define OVERFLOW_CHECK_HPP

#include <type_traits>
#include <limits>

#include "Buffer.hpp"

namespace ancient::internal
{

class OverflowCheck
{
public:
	template<typename T,typename U>
	static T sum(T a,U b)
	{
		static_assert(std::is_unsigned_v<T>);
		static_assert(std::is_unsigned_v<U>);

		T _b{T(b)};
		if (a>std::numeric_limits<T>::max()-_b)
			throw Buffer::OutOfBoundsError();
		return a+_b;
	}

	template<typename T,typename U,typename ...Args>
	static T sum(T a,U b,Args... args)
	{
		return sum(sum(a,b),args...);
	}
};

}

#endif
