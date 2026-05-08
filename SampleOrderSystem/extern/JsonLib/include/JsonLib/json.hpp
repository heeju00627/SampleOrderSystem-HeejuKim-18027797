#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <type_traits>
#include <iostream>
#include <cstdint>

namespace json {

class Value;

using Null   = std::monostate;
using Bool   = bool;
using Number = double;
using String = std::string;
using Array  = std::vector<Value>;
using Object = std::map<std::string, Value>;

enum class Type { Null, Bool, Number, String, Array, Object };

// ---- Exceptions ----

class Exception : public std::runtime_error {
public:
    explicit Exception(const std::string& msg) : std::runtime_error(msg) {}
};

class ParseError : public Exception {
public:
    ParseError(const std::string& msg, std::size_t pos)
        : Exception(msg + " (position " + std::to_string(pos) + ")")
        , pos_(pos) {}
    std::size_t position() const noexcept { return pos_; }
private:
    std::size_t pos_;
};

class TypeError : public Exception {
public:
    explicit TypeError(const std::string& msg) : Exception(msg) {}
};

// ---- Value ----

class Value {
public:
    Value() noexcept;
    Value(std::nullptr_t) noexcept;
    Value(bool v) noexcept;
    Value(const char* v);
    Value(const std::string& v);
    Value(std::string&& v) noexcept;
    Value(const Array& v);
    Value(Array&& v) noexcept;
    Value(const Object& v);
    Value(Object&& v) noexcept;

    template<typename T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
    Value(T v) noexcept : data_(Number(static_cast<double>(v))) {}

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    Value(T v) noexcept : data_(Number(static_cast<double>(v))) {}

    Value(const Value&)            = default;
    Value(Value&&) noexcept        = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&)      = default;

    Type type()     const noexcept;
    bool isNull()   const noexcept;
    bool isBool()   const noexcept;
    bool isNumber() const noexcept;
    bool isString() const noexcept;
    bool isArray()  const noexcept;
    bool isObject() const noexcept;

    bool               asBool()    const;
    double             asDouble()  const;
    int                asInt()     const;
    int64_t            asInt64()   const;
    const std::string& asString()  const;

    Array&             asArray();
    const Array&       asArray()   const;
    Object&            asObject();
    const Object&      asObject()  const;

    Value&       operator[](std::size_t index);
    const Value& operator[](std::size_t index) const;
    Value&       at(std::size_t index);
    const Value& at(std::size_t index) const;

    void push_back(const Value& v);
    void push_back(Value&& v);

    Array::iterator       begin();
    Array::iterator       end();
    Array::const_iterator begin()  const;
    Array::const_iterator end()    const;
    Array::const_iterator cbegin() const;
    Array::const_iterator cend()   const;

    Value&       operator[](const std::string& key);
    Value&       at(const std::string& key);
    const Value& at(const std::string& key) const;

    bool contains(const std::string& key) const noexcept;
    bool erase(const std::string& key);
    std::vector<std::string> keys() const;

    std::size_t size()  const noexcept;
    bool        empty() const noexcept;
    void        clear();

    bool operator==(const Value& o) const noexcept;
    bool operator!=(const Value& o) const noexcept;

    static Value parse(const std::string& text);
    static Value parseFile(const std::string& path);

    std::string dump(int indent = -1) const;
    bool        save(const std::string& path, int indent = 4) const;

    friend std::ostream& operator<<(std::ostream& os, const Value& v);

private:
    using Storage = std::variant<Null, Bool, Number, String, Array, Object>;
    Storage data_;

    void dumpTo(std::string& out, int indent, int depth) const;
    static void escapeString(std::string& out, const std::string& s);
    static void formatNumber(std::string& out, double v);
};

inline Value makeObject() { return Value(Object{}); }
inline Value makeArray()  { return Value(Array{});  }

using JsonValue = Value;

} // namespace json
