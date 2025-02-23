#pragma once


#include <cstdint>
#include <type_traits>
#include <wlib.hpp>

#include <CpputilsDebug.h>
#include <static_format.h>

namespace app {

class BaseModule
{
public:
	/**
	 * host to network function, for the values of this module
	 * returns the underlying value of the enum by using the underlying enum type.
	 * eg: class enum X : uint8_t {
	 *       Y = 4
	 *     };
	 *
	 *     returns uint8_t(4)
	 */
	template<class T,class=typename std::enable_if< std::is_enum<T>::value >::type>
	static auto hton( const T & value ) {
		using value_type = typename std::remove_cvref<decltype(value)>::type;
		using underlying_type = typename std::underlying_type<value_type>::type;
		return underlying_type(value);
	}

	template<class T,class=typename std::enable_if< !std::is_enum<T>::value >::type>
	static T hton( const T & value ) {
		return value;
	}

	template<class T,class=typename std::enable_if< std::is_enum<T>::value >::type>
	static void ntoh( wlib::blob::ConstMemoryBlob & blob, T & value ) {
		using value_type = typename std::remove_cvref<decltype(value)>::type;
		using underlying_type = typename std::underlying_type<value_type>::type;

		value = value_type(blob.extract_front<underlying_type>());
	}
};

} // namespace app
