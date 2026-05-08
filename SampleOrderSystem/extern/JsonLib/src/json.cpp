#include "JsonLib/json.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace json {

Value::Value() noexcept                  : data_(Null{})           {}
Value::Value(std::nullptr_t) noexcept    : data_(Null{})           {}
Value::Value(bool v) noexcept            : data_(Bool(v))          {}
Value::Value(const char* v)              : data_(String(v))        {}
Value::Value(const std::string& v)       : data_(String(v))        {}
Value::Value(std::string&& v) noexcept   : data_(String(std::move(v))) {}
Value::Value(const Array& v)             : data_(v)                {}
Value::Value(Array&& v) noexcept         : data_(std::move(v))     {}
Value::Value(const Object& v)            : data_(v)                {}
Value::Value(Object&& v) noexcept        : data_(std::move(v))     {}

Type Value::type() const noexcept { return static_cast<Type>(data_.index()); }

bool Value::isNull()   const noexcept { return std::holds_alternative<Null>(data_);   }
bool Value::isBool()   const noexcept { return std::holds_alternative<Bool>(data_);   }
bool Value::isNumber() const noexcept { return std::holds_alternative<Number>(data_); }
bool Value::isString() const noexcept { return std::holds_alternative<String>(data_); }
bool Value::isArray()  const noexcept { return std::holds_alternative<Array>(data_);  }
bool Value::isObject() const noexcept { return std::holds_alternative<Object>(data_); }

static const char* typeName(const Value& v) {
    switch (v.type()) {
        case Type::Null:   return "null";
        case Type::Bool:   return "bool";
        case Type::Number: return "number";
        case Type::String: return "string";
        case Type::Array:  return "array";
        case Type::Object: return "object";
    }
    return "unknown";
}

bool Value::asBool() const {
    if (!isBool()) throw TypeError(std::string("Expected bool, got ") + typeName(*this));
    return std::get<Bool>(data_);
}
double Value::asDouble() const {
    if (!isNumber()) throw TypeError(std::string("Expected number, got ") + typeName(*this));
    return std::get<Number>(data_);
}
int     Value::asInt()   const { return static_cast<int>(asDouble());     }
int64_t Value::asInt64() const { return static_cast<int64_t>(asDouble()); }

const std::string& Value::asString() const {
    if (!isString()) throw TypeError(std::string("Expected string, got ") + typeName(*this));
    return std::get<String>(data_);
}
Array& Value::asArray() {
    if (!isArray()) throw TypeError(std::string("Expected array, got ") + typeName(*this));
    return std::get<Array>(data_);
}
const Array& Value::asArray() const {
    if (!isArray()) throw TypeError(std::string("Expected array, got ") + typeName(*this));
    return std::get<Array>(data_);
}
Object& Value::asObject() {
    if (!isObject()) throw TypeError(std::string("Expected object, got ") + typeName(*this));
    return std::get<Object>(data_);
}
const Object& Value::asObject() const {
    if (!isObject()) throw TypeError(std::string("Expected object, got ") + typeName(*this));
    return std::get<Object>(data_);
}

Value& Value::operator[](std::size_t index)       { return asArray()[index]; }
const Value& Value::operator[](std::size_t index) const { return asArray()[index]; }

Value& Value::at(std::size_t index) {
    auto& arr = asArray();
    if (index >= arr.size())
        throw std::out_of_range("json::Value::at: index " + std::to_string(index) +
                                " out of range (size " + std::to_string(arr.size()) + ")");
    return arr[index];
}
const Value& Value::at(std::size_t index) const {
    const auto& arr = asArray();
    if (index >= arr.size())
        throw std::out_of_range("json::Value::at: index " + std::to_string(index) +
                                " out of range (size " + std::to_string(arr.size()) + ")");
    return arr[index];
}

void Value::push_back(const Value& v) { asArray().push_back(v);            }
void Value::push_back(Value&& v)      { asArray().push_back(std::move(v)); }

Array::iterator       Value::begin()        { return asArray().begin();  }
Array::iterator       Value::end()          { return asArray().end();    }
Array::const_iterator Value::begin()  const { return asArray().begin();  }
Array::const_iterator Value::end()    const { return asArray().end();    }
Array::const_iterator Value::cbegin() const { return asArray().cbegin(); }
Array::const_iterator Value::cend()   const { return asArray().cend();   }

Value& Value::operator[](const std::string& key) { return asObject()[key]; }

Value& Value::at(const std::string& key) {
    auto& obj = asObject();
    auto it = obj.find(key);
    if (it == obj.end())
        throw std::out_of_range("json::Value::at: key '" + key + "' not found");
    return it->second;
}
const Value& Value::at(const std::string& key) const {
    const auto& obj = asObject();
    auto it = obj.find(key);
    if (it == obj.end())
        throw std::out_of_range("json::Value::at: key '" + key + "' not found");
    return it->second;
}

bool Value::contains(const std::string& key) const noexcept {
    if (!isObject()) return false;
    return std::get<Object>(data_).count(key) > 0;
}
bool Value::erase(const std::string& key) {
    auto& obj = asObject();
    auto it = obj.find(key);
    if (it == obj.end()) return false;
    obj.erase(it);
    return true;
}
std::vector<std::string> Value::keys() const {
    const auto& obj = asObject();
    std::vector<std::string> result;
    result.reserve(obj.size());
    for (const auto& [k, _] : obj) result.push_back(k);
    return result;
}

std::size_t Value::size() const noexcept {
    if (isArray())  return std::get<Array>(data_).size();
    if (isObject()) return std::get<Object>(data_).size();
    return 0;
}
bool Value::empty() const noexcept { return size() == 0; }
void Value::clear() {
    if (isArray())  std::get<Array>(data_).clear();
    if (isObject()) std::get<Object>(data_).clear();
}

bool Value::operator==(const Value& o) const noexcept { return data_ == o.data_; }
bool Value::operator!=(const Value& o) const noexcept { return !(*this == o);    }

void Value::escapeString(std::string& out, const std::string& s) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    out += '"';
}

void Value::formatNumber(std::string& out, double v) {
    if (std::isnan(v) || std::isinf(v))
        throw Exception("JSON does not allow NaN or Infinity");
    if (v >= -9007199254740992.0 && v <= 9007199254740992.0) {
        auto i = static_cast<int64_t>(v);
        if (static_cast<double>(i) == v) { out += std::to_string(i); return; }
    }
    char buf[64];
    int len = std::snprintf(buf, sizeof(buf), "%.17g", v);
    out.append(buf, static_cast<std::size_t>(len));
}

void Value::dumpTo(std::string& out, int indent, int depth) const {
    switch (type()) {
        case Type::Null:   out += "null"; break;
        case Type::Bool:   out += std::get<Bool>(data_) ? "true" : "false"; break;
        case Type::Number: formatNumber(out, std::get<Number>(data_)); break;
        case Type::String: escapeString(out, std::get<String>(data_)); break;
        case Type::Array: {
            const auto& arr = std::get<Array>(data_);
            if (arr.empty()) { out += "[]"; break; }
            out += '[';
            for (std::size_t i = 0; i < arr.size(); ++i) {
                if (indent >= 0) { out += '\n'; out.append(static_cast<std::size_t>((depth + 1) * indent), ' '); }
                arr[i].dumpTo(out, indent, depth + 1);
                if (i + 1 < arr.size()) out += ',';
            }
            if (indent >= 0) { out += '\n'; out.append(static_cast<std::size_t>(depth * indent), ' '); }
            out += ']';
            break;
        }
        case Type::Object: {
            const auto& obj = std::get<Object>(data_);
            if (obj.empty()) { out += "{}"; break; }
            out += '{';
            std::size_t i = 0;
            for (const auto& [key, val] : obj) {
                if (indent >= 0) { out += '\n'; out.append(static_cast<std::size_t>((depth + 1) * indent), ' '); }
                escapeString(out, key);
                out += ':';
                if (indent >= 0) out += ' ';
                val.dumpTo(out, indent, depth + 1);
                if (++i < obj.size()) out += ',';
            }
            if (indent >= 0) { out += '\n'; out.append(static_cast<std::size_t>(depth * indent), ' '); }
            out += '}';
            break;
        }
    }
}

std::string Value::dump(int indent) const {
    std::string out; out.reserve(256);
    dumpTo(out, indent, 0);
    return out;
}

bool Value::save(const std::string& path, int indent) const {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;
    std::string text = dump(indent);
    ofs.write(text.data(), static_cast<std::streamsize>(text.size()));
    return ofs.good();
}

std::ostream& operator<<(std::ostream& os, const Value& v) { return os << v.dump(4); }

// ---- Parser ----

class Parser {
public:
    explicit Parser(const std::string& input) : input_(input), pos_(0) {}

    Value parse() {
        skipWs();
        Value result = parseValue();
        skipWs();
        if (pos_ != input_.size())
            throw ParseError("Unexpected trailing content", pos_);
        return result;
    }

private:
    const std::string& input_;
    std::size_t        pos_;

    void skipWs() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) ++pos_;
    }
    char peek() const {
        if (pos_ >= input_.size()) throw ParseError("Unexpected end of input", pos_);
        return input_[pos_];
    }
    char advance() {
        if (pos_ >= input_.size()) throw ParseError("Unexpected end of input", pos_);
        return input_[pos_++];
    }
    void expect(char c) {
        char got = advance();
        if (got != c) throw ParseError(std::string("Expected '") + c + "', got '" + got + "'", pos_ - 1);
    }
    bool startsWith(const char* s, std::size_t len) const {
        return pos_ + len <= input_.size() && input_.compare(pos_, len, s, len) == 0;
    }

    Value parseValue() {
        skipWs();
        char c = peek();
        if (c == '"') return parseString();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == 't' || c == 'f') return parseBool();
        if (c == 'n') return parseNull();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber();
        throw ParseError(std::string("Unexpected character '") + c + "'", pos_);
    }

    Value parseNull() {
        if (startsWith("null", 4)) { pos_ += 4; return Value(); }
        throw ParseError("Expected 'null'", pos_);
    }
    Value parseBool() {
        if (startsWith("true",  4)) { pos_ += 4; return Value(true);  }
        if (startsWith("false", 5)) { pos_ += 5; return Value(false); }
        throw ParseError("Expected 'true' or 'false'", pos_);
    }
    Value parseNumber() {
        std::size_t start = pos_;
        if (peek() == '-') ++pos_;
        if (pos_ >= input_.size() || !std::isdigit(static_cast<unsigned char>(input_[pos_])))
            throw ParseError("Expected digit in number", pos_);
        if (input_[pos_] == '0') {
            ++pos_;
            if (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_])))
                throw ParseError("Leading zero not allowed", pos_);
        } else {
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) ++pos_;
        }
        if (pos_ < input_.size() && input_[pos_] == '.') {
            ++pos_;
            if (pos_ >= input_.size() || !std::isdigit(static_cast<unsigned char>(input_[pos_])))
                throw ParseError("Expected digit after decimal point", pos_);
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) ++pos_;
        }
        if (pos_ < input_.size() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < input_.size() && (input_[pos_] == '+' || input_[pos_] == '-')) ++pos_;
            if (pos_ >= input_.size() || !std::isdigit(static_cast<unsigned char>(input_[pos_])))
                throw ParseError("Expected digit in exponent", pos_);
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) ++pos_;
        }
        try { return Value(std::stod(input_.substr(start, pos_ - start))); }
        catch (...) { throw ParseError("Invalid number", start); }
    }

    std::string parseRawString() {
        expect('"');
        std::string result;
        while (true) {
            if (pos_ >= input_.size()) throw ParseError("Unterminated string", pos_);
            unsigned char c = static_cast<unsigned char>(input_[pos_]);
            if (c == '"') { ++pos_; break; }
            if (c < 0x20) throw ParseError("Unescaped control character", pos_);
            if (c != '\\') { result += static_cast<char>(c); ++pos_; continue; }
            ++pos_;
            if (pos_ >= input_.size()) throw ParseError("Unexpected end in escape", pos_);
            char esc = input_[pos_++];
            switch (esc) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'u':  decodeUnicode(result); break;
                default:   throw ParseError(std::string("Unknown escape '\\") + esc + "'", pos_ - 1);
            }
        }
        return result;
    }

    void decodeUnicode(std::string& out) {
        uint32_t cp = readHex4();
        if (cp >= 0xD800 && cp <= 0xDBFF) {
            if (!startsWith("\\u", 2)) throw ParseError("Expected low surrogate", pos_);
            pos_ += 2;
            uint32_t low = readHex4();
            if (low < 0xDC00 || low > 0xDFFF) throw ParseError("Invalid low surrogate", pos_);
            cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
        }
        appendUtf8(out, cp);
    }

    uint32_t readHex4() {
        if (pos_ + 4 > input_.size()) throw ParseError("Incomplete \\uXXXX", pos_);
        uint32_t cp = 0;
        for (int i = 0; i < 4; ++i) {
            char c = input_[pos_++]; cp <<= 4;
            if      (c >= '0' && c <= '9') cp |= static_cast<uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') cp |= static_cast<uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') cp |= static_cast<uint32_t>(c - 'A' + 10);
            else throw ParseError("Invalid hex digit", pos_ - 1);
        }
        return cp;
    }

    static void appendUtf8(std::string& out, uint32_t cp) {
        if (cp < 0x80) { out += static_cast<char>(cp); }
        else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6)  & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    Value parseString() { return Value(parseRawString()); }

    Value parseArray() {
        expect('['); Array arr; skipWs();
        if (peek() == ']') { ++pos_; return Value(std::move(arr)); }
        while (true) {
            arr.push_back(parseValue()); skipWs();
            char c = peek();
            if (c == ']') { ++pos_; break; }
            if (c != ',') throw ParseError("Expected ',' or ']'", pos_);
            ++pos_;
        }
        return Value(std::move(arr));
    }

    Value parseObject() {
        expect('{'); Object obj; skipWs();
        if (peek() == '}') { ++pos_; return Value(std::move(obj)); }
        while (true) {
            skipWs();
            if (peek() != '"') throw ParseError("Expected string key", pos_);
            std::string key = parseRawString();
            skipWs(); expect(':');
            obj[std::move(key)] = parseValue();
            skipWs();
            char c = peek();
            if (c == '}') { ++pos_; break; }
            if (c != ',') throw ParseError("Expected ',' or '}'", pos_);
            ++pos_;
        }
        return Value(std::move(obj));
    }
};

Value Value::parse(const std::string& text) { Parser p(text); return p.parse(); }

Value Value::parseFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw Exception("Cannot open file: " + path);
    std::ostringstream oss; oss << ifs.rdbuf();
    if (ifs.fail() && !ifs.eof()) throw Exception("Failed to read file: " + path);
    return parse(oss.str());
}

} // namespace json
