#pragma once
#include <limits>
#include <cstdint>

namespace detail 
{
// The compiler issues a warning that "value" is unused when both sides of the conversion are the same type.
#pragma warning(push)
#pragma warning(disable: 4100)
	// http://stackoverflow.com/a/17251989
	template <typename T, typename U>
	bool CanTypeFitValue(const U value) 
	{
		const intmax_t botT = intmax_t(std::numeric_limits<T>::min());
		const intmax_t botU = intmax_t(std::numeric_limits<U>::min());
		const uintmax_t topT = uintmax_t(std::numeric_limits<T>::max());
		const uintmax_t topU = uintmax_t(std::numeric_limits<U>::max());
		return !((botT > botU && value < static_cast<U>(botT)) || (topT < topU && value > static_cast<U>(topT)));
	}
#pragma warning(pop)
}

template <typename Target, typename Source>
Target numeric_cast(Source s)
{
	static_assert(std::numeric_limits<Target>::is_integer && std::numeric_limits<Source>::is_integer,
		"numeric_cast only supports integral conversions");

	if (!detail::CanTypeFitValue<Target>(s))
	{
		throw std::runtime_error("Numeric cast would lose precision.");
	}

	return static_cast<Target>(s);
}
