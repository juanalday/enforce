# jag::enforce

Enforcements in C++ were defined by Petru Marginean and Andrei Alexandrescu in http://www.drdobbs.com/enforcements/184403864

The typical use case is 

```cpp
ENFORCE(variable)("Variable is null and it shouldn't!");
```

The macro will in turn expand to something like:

```cpp

*MakeEnforcer<DefaultPredicate, DefaultRaiser>(
    (variable), "Expression 'variable' failed in 'blah.cpp', line: 7"))
("Variable is null and it shouldn't!");
```

The underlying enforce object will store a reference to the passed object and then, if validation failed, will stream the arguments passed to it's operator().

While the technique works brilliantly, complex/time consuming operations could really slow down the codebase even when they are not needed.

ENFORCE(variable)("Variable is null: ")(call_to_expensive_function());

```cpp

*MakeEnforcer<DefaultPredicate, DefaultRaiser>(
    (variable), "Expression 'variable' failed in 'blah.cpp', line: 7"))
("Variable is null")
(call_to_expensive_function());
```

In other words: every argument sent to the ENFORCE macro is evaluated, thus wasting cycles and in some cases, causing side effects.

```cpp
std::string call_to_expensive_function(char const* ptr)
{
 // ... expensive call to store the contents of ptr in a database
     store_value_of_ptr_in_error_database(ptr);
 // ... expensive call to retrieve the list of valid values from a database
     return call_to_retrieve_values_from_database();
}

struct MyPredicate
{
    static bool Wrong(std::string const& obj) {return obj == "hello";}
};

char const* variable = "world";
*MakeEnforcer<MyPredicate, DefaultRaiser>(
    (variable), "Expression 'variable' failed in 'blah.cpp', line: 7"))
("Variable is 'hello'. Her is a list of possible valid values: ")
(call_to_expensive_function(variable));
```

In this example, the enforcement fails only if the value of the string is "hello".

However, we always invoke the expensive function, which as a side effect will store every value as a false positive.

# Functionality


we can define a better enforcer. We'd like to keep the same functionality of the original enforce code, but without the penalty of execution.


## default predicate + default raiser

```cpp
enforce(ptr);

```

## custom predicate
```cpp
enforce("hello"s, [](auto const& value)->bool {return value == "hello";});
```

## custom error

```cpp
enforce(false,[](auto && value, auto& buffer) {buffer << Not allowed to use false here;});
enforce(false,[](auto && value) {return "Not allowed to use false here";});
```

## custom raiser
In this example, I have defined a validator function and an output function. 

```cpp
enforce(false, [](std::string const& msg) {throw std::runtime_error(msg); });
```

# Variant
