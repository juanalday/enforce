#include "enforce.hpp"
#include <iostream>
#include <string>
#include <type_traits>
#include <gtest/gtest.h>


struct tester {
    tester()
    {
    }
    tester(tester const&)
    {
		throw std::runtime_error("Copy constructor invoked");
    }
    tester& operator=(tester const&)
    {
		throw std::runtime_error("Assignment operator invoked");
    }
    tester(tester&&)
    {
		throw std::runtime_error("Move copy constructor invoked");
    }
    tester& operator=(tester&&) 
    {
		throw std::runtime_error("Move assignment operator invoked");
    }

	static tester make() { return tester(); }
	operator bool() const {return true; }
};

using jag::enforce;
using namespace jag::detail;

TEST(enforce, can_validate)
{
	EXPECT_FALSE(can_validate(true)); // No validator function
	EXPECT_FALSE(can_validate(false)); // No validator function
	EXPECT_FALSE(can_validate([] {return 1; })); // Standalone validator, no arguments, but doesn't return bool
	EXPECT_TRUE(can_validate([] {return true; })); // Standalone validator, no arguments, returning boolean
	EXPECT_TRUE(can_validate([] {return false; })); // Standalone validator, no arguments, returning boolean
	EXPECT_TRUE(can_validate(42, [](auto) {return true; })); // Standalone validator, single argument of same type as first argument to can_validate, returning boolean
	EXPECT_TRUE(can_validate(42, [](auto) {return false; })); // Standalone validator, single argument of same type as first argument to can_validate, returning boolean
	EXPECT_TRUE(can_validate(42, [](int) {return false; })); // Standalone validator, single argument of same type as first argument to can_validate, returning boolean
	EXPECT_TRUE(can_validate(42, [](double) {return false; })); // Standalone validator, single argument of same type as first argument to can_validate, returning boolean
	EXPECT_FALSE(can_validate(42, [](int, double) {return true; })); // Multiargument validator. 

	// Multiple arguments
	EXPECT_FALSE(can_validate(true, [] {return 0; }));
	EXPECT_FALSE(can_validate(false, [] {return 0; })); 
	EXPECT_FALSE(can_validate([] {return 1; }, [] {return 1; }));
	EXPECT_TRUE(can_validate(false, [] {return true; })); 
	EXPECT_TRUE(can_validate(false, [](auto) {return true; })); 
	EXPECT_TRUE(can_validate(false, [] {return true; }, [] { })); 
	EXPECT_TRUE(can_validate(false, [](auto) {}, [] {return true; })); 

	EXPECT_TRUE(can_validate([] {return false; }, [] {}));
	EXPECT_TRUE(can_validate(42, [](auto) {return true; }, [] {}));
	EXPECT_TRUE(can_validate(42, [] {}, [](auto) {return true; }));
	EXPECT_TRUE(can_validate(42, [] {}, [](auto) {return false; }));
	EXPECT_TRUE(can_validate(42, [] {}, [](int) {return false; }));
	EXPECT_TRUE(can_validate(42, [] {}, [](double) {return false; }));
	EXPECT_FALSE(can_validate(42, [] {}, [](int, double) {return true; }));
}

TEST(enforce, no_arguments)
{
	EXPECT_NO_THROW(enforce(true));
	EXPECT_THROW(enforce(false), std::runtime_error);
}

TEST(enforce, validator_follows)
{
	EXPECT_NO_THROW(enforce(true, [] {}, [] {return true; }));
}
TEST(enforce, custom_validator)
{
	using namespace std::literals;
	int const i = 4;
	EXPECT_NO_THROW(enforce(4, [](int v) {return v == 4; }));
	EXPECT_NO_THROW(enforce(4, [](auto v)->bool {return v == 4; }));
	EXPECT_NO_THROW(enforce(4, [](auto const& v)->bool {return v == 4; }));
	EXPECT_NO_THROW(enforce(4, [](auto&& v)->bool {return v == 4; }));
	EXPECT_NO_THROW(enforce(i, [](auto v)->bool {return v == 4; }));
	EXPECT_NO_THROW(enforce(i, [](auto const& v)->bool {return v == 4; }));
	EXPECT_NO_THROW(enforce(i, [](auto&& v)->bool {return v == 4; }));
	EXPECT_NO_THROW(enforce("hello"s, [](auto&& v)->bool {return v == "hello"; }));
	EXPECT_THROW(enforce(100, [](auto v)->bool {return v == 1; }), std::runtime_error);
	EXPECT_THROW(enforce(100, [](auto const& v)->bool {return v == 1; }), std::runtime_error);
	EXPECT_THROW(enforce(100, [](auto&& v)->bool {return v == 1; }), std::runtime_error);
}

TEST(enforce, mutable_validator)
{
	EXPECT_NO_THROW(enforce(1, 
		[](auto&& v)->bool {v = 10; return true; }
		, [](auto&& v)->bool {return v == 10; }
	));
	EXPECT_THROW(enforce(1, 
		[](auto&& v)->bool {v = 10; return true; }
		, [](auto&& v)->bool {return v == 1; }
	), std::runtime_error);
	
}

TEST(enforce, custom_error_message)
{
	try {
		enforce(false, [](auto&& value, auto& buffer) {buffer << "Value is " << std::boolalpha << value; });
	}
	catch (std::runtime_error const& e)
	{
		auto msg = e.what();
		EXPECT_STREQ(msg, "Value is false");
	}
	try {
		enforce(false, [](auto&& value) {return "Value is false"; });
	}
	catch (std::runtime_error const& e)
	{
		auto msg = e.what();
		EXPECT_STREQ(msg, "Value is false");
	}
}

TEST(enforce, custom_raiser)
{
	try {
		enforce(false, [](auto msg) {throw std::runtime_error("Value is false"); });
	}
	catch (std::runtime_error const& e)
	{
		auto msg = e.what();
		EXPECT_STREQ(msg, "Value is false");
	}
}
TEST(enforce, perfect_forwarding)
{
	tester value;
	tester const const_value;
	enforce(tester());
	EXPECT_NO_THROW(enforce(tester()));
	EXPECT_NO_THROW(enforce(value));
	EXPECT_NO_THROW(enforce(const_value));
	EXPECT_NO_THROW(tester&& t = enforce(tester()));
	EXPECT_THROW(auto t = enforce(tester()), std::runtime_error);
	EXPECT_THROW(tester t = enforce(tester()), std::runtime_error);
	EXPECT_THROW(tester t = enforce(tester()), std::runtime_error);
}

TEST(enforce, macro_calling)
{
	ENFORCE(true);
}