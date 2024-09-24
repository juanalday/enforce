#ifndef JAG_ENFORCE_HPP
#define JAG_ENFORCE_HPP

#include <functional>
#include <type_traits>
#include <utility>
#include <sstream>
#include <string>
#include <concepts>


#include <iostream>

#define ENFORCE_PP_CAT(a, b) ENFORCE_PP_CAT_I(a, b)
#define ENFORCE_PP_CAT_I(a, b) a ## b
#define ENFORCE_PP_STRINGIZE(text) ENFORCE_PP_STRINGIZE_I(text)
#define ENFORCE_PP_STRINGIZE_I(...) #__VA_ARGS__

#define PROCESS_ELEMENTS(first, ...) \
       [&](std::ostringstream& ENFORCE_PP_CAT(tmpBuffer, __LINE__) ) {ENFORCE_PP_CAT(tmpBuffer, __LINE__) << first __VA_OPT__(<< __VA_ARGS__) ;}

#define ENFORCE(exp, ...) \
  jag::enforce(exp  __VA_OPT__(, PROCESS_ELEMENTS(__VA_ARGS__) ), [](std::ostringstream& buffer) {buffer << ":Expression '" ENFORCE_PP_STRINGIZE(exp) "' failed";} )

namespace jag {
	namespace detail {

		template <class T>
		concept same_as_bool_impl = std::same_as<T, bool>;
		template <class T>
		concept same_as_bool = same_as_bool_impl<T> && requires(T && t) {
			{ !static_cast<T&&>(t) } -> same_as_bool_impl;

		};

		template <class F, class... Args>
		concept validator = std::regular_invocable<F, Args...>&& same_as_bool<std::invoke_result_t<F, Args...>>;
		template<class F, class... Args>
		concept stringable = std::regular_invocable<F, Args...>&& std::convertible_to<std::invoke_result_t<F, Args...>, std::string>;
		template<class F, class... Args>
		concept appendable = std::regular_invocable<F, Args..., std::ostringstream&>&& std::is_same_v< std::invoke_result_t<F, Args..., std::ostringstream&>, void>;

		template<class F, class... Args>
		concept appender = stringable<F, Args...> || appendable<F, Args...>;

		template<class F>
		concept raiser = std::regular_invocable<F, std::string >&& std::convertible_to< std::invoke_result_t<F, std::string>, void>;

		template<typename F>
		constexpr bool has_validator() {return validator<F>;}

		template<typename T, typename F, typename... Args>
		constexpr bool has_validator() {return validator<F, T> || validator <F> || has_validator<T, Args...>();}

	


		template<typename T, typename F>
		bool validate_impl(T&& t, F&& f)
		{
			if constexpr (validator<F>)
			{
				return f();
			}
			else if constexpr (validator<F, T>)
			{
				return f(std::forward<T>(t));
			}
			return true;
		}
		template<typename T, typename... Args>
		bool validate(T&& t, Args&&... args) {
			if constexpr (has_validator<T, Args...>()) // So in my sequence, there is a validator..
			{
				if constexpr (validator<T>)
					if (!t()) // T itself might be a validator. We will test it here
						return false;

				return (... && validate_impl(t, std::forward<Args>(args)));  // Unary right fold
			}

			if constexpr (std::convertible_to<T, bool>) // If there is no validator, but T is convertible to bool, that is the result...
				return static_cast<bool>(t);
			return false;
		}


		template<typename T, typename... Args>
		constexpr bool wrong(T&& t, Args&& ... args) {
			return !validate(std::forward<T>(t), std::forward<Args>(args)...);
		}

		template<typename T, typename F>
		constexpr void append_impl(std::ostringstream& buffer, T&& t, F&& f) {
			if constexpr (stringable<F>)
			{
				buffer << f();
			}
			if constexpr (appendable<F>)
			{
				f(buffer);
			}
			else if constexpr (stringable<F, T>)
			{
				buffer << f(std::forward<T>(t));
			}
	
			else if constexpr (appendable<F, T>)
			{
				f(std::forward<T>(t), buffer);
			}

		}
		template<typename T, typename... Args>
		std::string append(T&& t, Args&& ... args) {
			std::ostringstream buffer;
			if constexpr (stringable<T>)
			{
				buffer << t();
			}
			if constexpr (appendable<T>)
			{
				t(buffer);
			}
			(..., append_impl(buffer, t, std::forward<Args>(args)));  // Unary right fold
			if (buffer.tellp() == std::streampos(0))
				buffer << "Expression has failed";
			return buffer.str();
		}

		template<typename F>
		void raise_impl(std::string const& msg, F&& f)
		{
			if constexpr (raiser<F>)
			{
				f(msg);
			}
		}
		template<typename T, typename... Args>
		void raise(std::string const& msg, T&& t, Args&& ... args) {
			(raise_impl(msg, std::forward<T>(t)), ..., raise_impl(msg, std::forward<Args>(args)));
			throw std::runtime_error(msg); // IF no thrower function exists, I send a runtime_error myself
		}

	}
	template<typename T, typename... Args>
	decltype(auto) enforce(T&& t, Args&& ... args) {
		if (detail::wrong(std::forward<T>(t), std::forward<Args>(args)...)) {
			std::string const msg = detail::append(t, std::forward<Args>(args)...);
			detail::raise(msg, std::forward<T>(t), std::forward<Args>(args)...);
		}
		return std::forward<T>(t);
	}
} // end of namespace jag

#endif // JAG_ENFORCE_HPP
