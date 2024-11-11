

#include "my_json11.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <limits>

namespace my_json11 {
static const int max_depth = 200;   // 最大深度

using std::string;
using std::vector;
using std::map;
using std::make_shared;
using std::initializer_list;
using std::move;

/* Helper for representing null - just a do-nothing struct, plus comparison
 * operators so the helpers in JsonValue work. We can't use nullptr_t because
 * it may not be orderable.
 * 用于表示 null 的辅助结构 - 只是一个无操作的结构，加上比较运算符，
 * 以便 JsonValue 中的辅助程序工作。我们不能使用 nullptr_t，因为它可能不可排序。
 */
struct NullStruct {
    bool operator==(NullStruct) const { return true; }
    bool operator<(NullStruct) const { return false; }
};

/*
 * Serialization
 */

static void dump(NullStruct, string &out) {
    out += "null";
}

static void dump(double value, string& out) {
    if(std::isfinite(value)) {  // 判断 value 是否是有限的
        char buf[32];
        snprintf(buf, sizeof buf, "%.17g", value);  // 将 value 格式化为字符串, "%.17g" 表示最多 17 位有效数字
        out += buf;
    } else {
        out += "null";
    }
}

static void dump(int value, string& out) {
    char buf[32];
    snprintf(buf, sizeof buf, "%d", value);  // 将 value 格式化为字符串, "%d" 表示十进制整数
    out += buf;
}

static void dump(bool value, string& out) {
    out += value ? "true" : "false";
}

// 静态 dump 函数，
static void dump(const string& value, string& out) {
    out += '"'; // 添加双引号
    for( size_t i = 0; i<value.length(); i++ ) {
        const char ch = value[i];
        if(ch=='\\'){
            out += "\\\\"; // 添加双反斜杠
        }else if(ch=='"'){
            out += "\\\""; // 添加反斜杠和双引号
        }else if(ch=='\b'){
            out += "\\b";  // 添加退格符
        }else if(ch=='\f'){
            out += "\\f";  // 添加换页符
        }else if(ch=='\n'){
            out += "\\n";  // 添加换行符
        }else if(ch=='\r'){
            out += "\\r";  // 添加回车符
        }else if(ch=='\t'){
            out += "\\t";  // 添加制表符
        }else if(static_cast<uint8_t>(ch)<=0x1f){
            char buf[8];
            snprintf(buf, sizeof buf, "\\u%04x", ch);  // 将 ch 格式化为 4 位十六进制数, "\\u%04x" 表示 4 位十六进制数
            out += buf;
        }else if(static_cast<uint8_t>(ch)==0xe2 && static_cast<uint8_t>(value[i+1])==0x80 
                && static_cast<uint8_t>(value[i+2])==0xa8){ // 判断是否是行分隔符
            out += "\\u2028";  // 添加行分隔符，U+2028 表示行分隔符
            i += 2;
        }else if(static_cast<uint8_t>(ch)==0xe2 && static_cast<uint8_t>(value[i+1])==0x80 
                && static_cast<uint8_t>(value[i+2])==0xa9){ // 判断是否是段分隔符     
            out += "\\u2029";  // 添加段分隔符
        }else{
            out += ch;  // 添加 ch
        }
    }
    out += '"';  // 添加双引号
}

static void dump(const Json::array& values, string& out) {
    bool first = true;
    out += "[";
    for(const auto& value: values) {
        if(!first) {
            out += ", ";
        }
        value.dump(out);  // 递归调用 dump 函数
        first = false;
    }
    out += "]";
}

static void dump(const Json::object& values, string& out) {
    bool first = true;
    out += "{";
    for(const auto& kv: values) {
        if(!first) {
            out += ", ";
        }
        dump(kv.first, out);  // 将键值对的键转换为字符串
        out += ": ";
        kv.second.dump(out);  // 将键值对的值转换为string
        first = false;
    }
    out += "}";
}

void Json::dump(string& out) const {
    m_ptr->dump(out);  // 调用 JsonValue 的 dump 函数
}

/*
 * Value wrappers: 封装值
 */
template <Json::Type tag, typename T>   // tag 表示类型，T 表示值的类型
class Value: public JsonValue {
protected:

    // Constructors
    explicit Value(const T& value): m_value(value) {}
    explicit Value(T&& value): m_value(move(value)) {}

    // Get type tag
    Json::Type type() const override {  // override 表示重写基类的虚函数
        return tag;
    }

    // Comparisons
    bool equals(const JsonValue* other) const override {
        return m_value == static_cast<const Value<tag, T>*>(other)->m_value;    // 比较两个值是否相等
    }
    
    bool less(const JsonValue* other) const override {
        return m_value < static_cast<const Value<tag, T>*>(other)->m_value;    // 比较两个值是否相等
    }

    const T m_value;  // 值
    void dump(string& out) const override { 
        my_json11::dump(m_value, out);  // 调用静态 dump 函数
    }
    
};

class JsonDouble final: public Value<Json::NUMBER, double> {
    double number_value() const override { return m_value; }
    int int_value() const override { return static_cast<int>(m_value); }
    bool equals(const JsonValue* other) const override {
        return m_value < other->number_value();
    }
    bool less(const JsonValue* other) const override {
        return m_value == other->number_value();
    }
public:
    explicit JsonDouble(double value) : Value(value) {}
};

class JsonInt final: public Value<Json::NUMBER, int> {
    double number_value() const override { return m_value; }
    int int_value() const override { return m_value; }
    bool equals(const JsonValue* other) const override {
        return m_value == other->number_value();
    }
    bool less(const JsonValue* other) const override {
        return m_value < other->number_value();
    }
public:
    explicit JsonInt(int value) : Value(value) {}
};

class JsonBoolean final: public Value<Json::BOOL, bool> {
    bool bool_value() const override { return m_value; }
public:
    explicit JsonBoolean(bool value) : Value(value) {}  // explicit 显示构造函数, 例如：JsonBoolean b = true; 是错误的
};

class JsonString final : public Value<Json::STRING, string> {
    const string &string_value() const override { return m_value; }
public:
    explicit JsonString(const string &value) : Value(value) {}
    explicit JsonString(string &&value)      : Value(move(value)) {}
};

class JsonArray final : public Value<Json::ARRAY, Json::array> {
    const Json::array &array_items() const override { return m_value; }
    const Json & operator[](size_t i) const override;
public:
    explicit JsonArray(const Json::array &value) : Value(value) {}
    explicit JsonArray(Json::array &&value)      : Value(move(value)) {}
};

class JsonObject final : public Value<Json::OBJECT, Json::object> {
    const Json::object &object_items() const override { return m_value; }
    const Json & operator[](const string &key) const override;
public:
    explicit JsonObject(const Json::object &value) : Value(value) {}
    explicit JsonObject(Json::object &&value)      : Value(move(value)) {}
};

class JsonNull final: public Value<Json::NUL, NullStruct> {
public:
    JsonNull(): Value({}) {}
};


/*
 * Static globals - static-init-safe: 静态全局变量 - 静态初始化安全
 */
// null, t, f, empty_string, empty_vector, empty_map 用 const 修饰，表示不可修改且唯一
struct Statics {
    const std::shared_ptr<JsonValue> null = make_shared<JsonNull>();
    const std::shared_ptr<JsonValue> t = make_shared<JsonBoolean>(true);
    const std::shared_ptr<JsonValue> f = make_shared<JsonBoolean>(false);
    const string empty_string;
    const vector<Json> empty_vector;
    const map<string, Json> empty_map;
    Statics() {}
};

static const Statics& statics() {   // 静态函数，返回静态变量
    static const Statics s {};  // static const 修饰，表示只初始化一次
    return s;
}

static const Json & static_null() {
    // This has to be separate, not in Statics, because Json() accesses statics().null.
    // 这必须是单独的，而不是在 Statics 中，因为 Json() 访问 statics().null。
    static const Json json_null;
    return json_null;
}


/*
 * Constructors for Json class: Json 类的构造函数
 */
Json::Json() noexcept                  : m_ptr(statics().null) {}
Json::Json(std::nullptr_t) noexcept    : m_ptr(statics().null) {}
Json::Json(double value)               : m_ptr(make_shared<JsonDouble>(value)) {}
Json::Json(int value)                  : m_ptr(make_shared<JsonInt>(value)) {}
Json::Json(bool value)                 : m_ptr(value ? statics().t : statics().f) {}
Json::Json(const string &value)        : m_ptr(make_shared<JsonString>(value)) {}
Json::Json(string &&value)             : m_ptr(make_shared<JsonString>(move(value))) {}
Json::Json(const char * value)         : m_ptr(make_shared<JsonString>(value)) {}
Json::Json(const Json::array &values)  : m_ptr(make_shared<JsonArray>(values)) {}
Json::Json(Json::array &&values)       : m_ptr(make_shared<JsonArray>(move(values))) {}
Json::Json(const Json::object &values) : m_ptr(make_shared<JsonObject>(values)) {}
Json::Json(Json::object &&values)      : m_ptr(make_shared<JsonObject>(move(values))) {}

/*
 * Accessors: 访问器
 */
Json::Type Json::type()                           const { return m_ptr->type();         }
double Json::number_value()                       const { return m_ptr->number_value(); }
int Json::int_value()                             const { return m_ptr->int_value();    }
bool Json::bool_value()                           const { return m_ptr->bool_value();   }
const string & Json::string_value()               const { return m_ptr->string_value(); }
const vector<Json> & Json::array_items()          const { return m_ptr->array_items();  }
const map<string, Json> & Json::object_items()    const { return m_ptr->object_items(); }
const Json & Json::operator[] (size_t i)          const { return (*m_ptr)[i];           }
const Json & Json::operator[] (const string &key) const { return (*m_ptr)[key];         }

// 问题：为什么下面这些函数返回常量值？
double                    JsonValue::number_value()              const { return 0; }
int                       JsonValue::int_value()                 const { return 0; }
bool                      JsonValue::bool_value()                const { return false; }
const string &            JsonValue::string_value()              const { return statics().empty_string; }
const vector<Json> &      JsonValue::array_items()               const { return statics().empty_vector; }
const map<string, Json> & JsonValue::object_items()              const { return statics().empty_map; }
const Json &              JsonValue::operator[] (size_t)         const { return static_null(); }
const Json &              JsonValue::operator[] (const string &) const { return static_null(); }

const Json& JsonObject::operator[] (const string& key) const {
    auto iter = m_value.find(key);
    return (iter == m_value.end()) ? static_null() : iter->second;
}
const Json & JsonArray::operator[] (size_t i) const {
    if (i >= m_value.size()) return static_null();
    else return m_value[i];
}

/*
 * Comparison operators: 比较运算符
 */

bool Json::operator==(const Json& rhs) const {
    if (m_ptr == rhs.m_ptr) return true;    // 指针相等，返回 true
    if (m_ptr->type() != rhs.m_ptr->type()) return false;   // 类型不同，返回 false
    return m_ptr->equals(rhs.m_ptr.get());  // get() 函数返回智能指针指向的原始指针， equals 函数比较两个值是否相等
}

bool Json::operator< (const Json &other) const {
    if (m_ptr == other.m_ptr)
        return false;
    if (m_ptr->type() != other.m_ptr->type())
        return m_ptr->type() < other.m_ptr->type(); // 比较类型

    return m_ptr->less(other.m_ptr.get());
}

/*
 * Parsing: 解析
 */

/*
 * esc(c)
 * Format char c suitable for printing in an error message: 格式化字符 c，以便在错误消息中打印
 */

static inline string esc(char c) {  // inline: 内联函数, 表示将函数体直接插入到调用函数的地方
    char buf[12];
    if(static_cast<uint8_t>(c) >= 0x20 && static_cast<uint8_t>(c) <= 0x7f) {    // 判断是否是可打印字符
        snprintf(buf, sizeof buf, "'%c' (%d)", c, c);   // '%c' (%d) 表示字符和 ASCII 码
    }else{
        snprintf(buf, sizeof buf, "(%d)", c);  // (%d) 表示 ASCII 码
    }
    return string(buf);
}

static inline bool in_range(long x, long lower, long upper) {
    return (x >= lower && x <= upper);  // 判断 x 是否在 [lower, upper] 范围内
}

namespace {

/*
 * JsonParser: 解析 JSON 字符串
 * Object that tracks all state of an in-progress parse: 跟踪正在进行的解析的所有状态的对象
 */

// 问题：JsonParser 是干什么的？
struct JsonParser final {
    // State: 状态
    const string& str;  // JSON 字符串
    size_t i;           // 当前解析的位置
    string& err;        // 错误信息
    bool failed;        // 是否解析失败
    const JsonParse strategy;  // 解析策略: STANDARD, COMMENTS

    // fail(msg, err_ret): 解析失败，返回 err_ret
    // Mark this parse as failed: 将此解析标记为失败
    Json fail(string&& msg) {
        return fail(move(msg), Json());
    }

    template <typename T>
    T fail(string&& msg, const T err_ret) {
        if(!failed) {
            err = std::move(msg);
        }
        failed = true;
        return err_ret;
    }

    // consume_whitespace(): 跳过空白字符
    // Advance until the current character is non-whitespace and non-comment: 前进，直到当前字符不是空白字符和注释
    void consume_whitespace() {
        while(str[i] == ' ' || str[i] == '\r' || str[i] == '\n' || str[i] == '\t') {
            i++;
        }
    }

    // consume_comment(): 跳过注释
    // Advance comments (c-style inline and multiline): 前进注释（C 风格的内联和多行注释）
    bool consume_comment() {
        bool comment_found = false;
        if(str[i] == '/') {
            ++i;
            if(i == str.size()) {
                return fail("unexpected end of input after start of comment", false);   // 返回 false，同时设置错误信息
            }else if(str[i] == '/') {  // inline comment
                ++i;
                // advance until next line, or end of input
                while(i < str.size() && str[i] != '\n') {
                    ++i;
                }
                comment_found = true;
            }else if(str[i] == '*') {  // multiline comment
                ++i;
                if(i > str.size() - 2) {
                    return fail("unexpected end of input inside multi-line comment", false);  // 返回 false，同时设置错误信息
                }
                while(!(str[i] == '*' && str[i+1] == '/')) {
                    ++i;
                    if(i > str.size() - 2) {
                        return fail("unexpected end of input inside multi-line comment", false);  // 返回 false，同时设置错误信息
                    }
                }
                i += 2;
                comment_found = true;
            }else{
                return fail("malformed comment", false);  // 返回 false，同时设置错误信息
            }
        }
        return comment_found;
    }

    // consume_garbage(): 跳过空白字符和注释
    // Advance until the current character is non-whitespace and non-comment: 前进，直到当前字符不是空白字符和注释
    void consume_garbage() {
        consume_whitespace();   // 先跳过空白字符
        if(strategy == JsonParse::COMMENTS) {   // 如果解析策略是 COMMENTS, 问题：解析策略什么时候设置
            bool comment_found = false;
            do {
                comment_found = consume_comment();  // 跳过注释
                if(failed) return;
                consume_whitespace();  // 再次跳过空白字符
            }while(comment_found);
        }
    }

    // get_next_token(): 获取下一个非空白字符
    // Return the next non-whitespace character. If the end of the input is reached,
    // flag an error and return 0
    char get_next_token() {
        consume_garbage();  // 跳过空白字符和注释
        if(failed) return static_cast<char>(0);  // 如果解析失败，返回 0
        if(i == str.size()) {
            return fail("unexpected end of input", static_cast<char>(0));  // 返回 0，同时设置错误信息
        }
        return str[i++];
    }

    // encode_utf8(pt, out): 将 pt 编码为 UTF-8，并添加到 out
    void encode_utf8(long pt, string& out) {
        if(pt < 0) {
            return;
        }
        if(pt < 0x80) {
            out += static_cast<char>(pt);
        }else if(pt < 0x800) {
            out += static_cast<char>((pt >> 6) | 0xC0);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        }else if(pt < 0x10000) {
            out += static_cast<char>((pt >> 12) | 0xE0);
            out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        }else{
            out += static_cast<char>((pt >> 18) | 0xF0);
            out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80);
            out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        }
    }

    // parse_string(): 解析字符串
    // Parse a string, starting at the current position: 从当前位置开始解析字符串
    string parse_string() { // TODO: 具体原理未看
        string out;
        long last_escaped_codepoint = -1;
        while (true) {
            if (i == str.size())
                return fail("unexpected end of input in string", "");

            char ch = str[i++];

            if (ch == '"') {
                encode_utf8(last_escaped_codepoint, out);
                return out;
            }

            if (in_range(ch, 0, 0x1f))
                return fail("unescaped " + esc(ch) + " in string", "");

            // The usual case: non-escaped characters
            if (ch != '\\') {
                encode_utf8(last_escaped_codepoint, out);
                last_escaped_codepoint = -1;
                out += ch;
                continue;
            }

            // Handle escapes
            if (i == str.size())
                return fail("unexpected end of input in string", "");

            ch = str[i++];

            if (ch == 'u') {
                // Extract 4-byte escape sequence
                string esc = str.substr(i, 4);
                // Explicitly check length of the substring. The following loop
                // relies on std::string returning the terminating NUL when
                // accessing str[length]. Checking here reduces brittleness.
                if (esc.length() < 4) {
                    return fail("bad \\u escape: " + esc, "");
                }
                for (size_t j = 0; j < 4; j++) {
                    if (!in_range(esc[j], 'a', 'f') && !in_range(esc[j], 'A', 'F')
                            && !in_range(esc[j], '0', '9'))
                        return fail("bad \\u escape: " + esc, "");
                }

                long codepoint = strtol(esc.data(), nullptr, 16);

                // JSON specifies that characters outside the BMP shall be encoded as a pair
                // of 4-hex-digit \u escapes encoding their surrogate pair components. Check
                // whether we're in the middle of such a beast: the previous codepoint was an
                // escaped lead (high) surrogate, and this is a trail (low) surrogate.
                if (in_range(last_escaped_codepoint, 0xD800, 0xDBFF)
                        && in_range(codepoint, 0xDC00, 0xDFFF)) {
                    // Reassemble the two surrogate pairs into one astral-plane character, per
                    // the UTF-16 algorithm.
                    encode_utf8((((last_escaped_codepoint - 0xD800) << 10)
                                 | (codepoint - 0xDC00)) + 0x10000, out);
                    last_escaped_codepoint = -1;
                } else {
                    encode_utf8(last_escaped_codepoint, out);
                    last_escaped_codepoint = codepoint;
                }

                i += 4;
                continue;
            }

            encode_utf8(last_escaped_codepoint, out);
            last_escaped_codepoint = -1;

            if (ch == 'b') {
                out += '\b';
            } else if (ch == 'f') {
                out += '\f';
            } else if (ch == 'n') {
                out += '\n';
            } else if (ch == 'r') {
                out += '\r';
            } else if (ch == 't') {
                out += '\t';
            } else if (ch == '"' || ch == '\\' || ch == '/') {
                out += ch;
            } else {
                return fail("invalid escape character " + esc(ch), "");
            }
        }
    }

    // parse_number(): 解析数字
    // parse a double.
    Json parse_number() {
        size_t start_pos = i;
        
        if(str[i] == '-') {
            i++;        
        }

        // Integer part
        if(str[i] == '0') {
            i++;
            if(in_range(str[i], '0', '9')) {
                return fail("leading 0s not followed by '.'", 0);
            }
        }else if(in_range(str[i], '1', '9')) {
            i++;
            while(in_range(str[i], '0', '9')) {
                i++;
            }
        }else{
            return fail("invalid " + esc(str[i]) + " in number");    
        }

        if(str[i] != '.' && str[i] != 'e' && str[i] != 'E' 
                && (i - start_pos) <= static_cast<size_t>(std::numeric_limits<int>::digits10)) {
            return std::atoi(str.c_str() + start_pos);  // 将字符串转换为整数
        }

        // Decimal part: 小数部分
        if(str[i] == '.') {
            i++;
            if(!in_range(str[i], '0', '9')) {
                return fail("at least one digit required in decimal part", 0);
            }
            while(in_range(str[i], '0', '9')) {
                i++;
            }
        }

        // Exponent part: 指数部分
        if(str[i] == 'e' || str[i] == 'E') {
            i++;
            if(str[i] == '+' || str[i] == '-') {
                i++;
            }
            if(!in_range(str[i], '0', '9')) {
                return fail("at least one digit required in exponent", 0);
            }
            while(in_range(str[i], '0', '9')) {
                i++;
            }
        }
        return std::strtod(str.c_str() + start_pos, nullptr);  // 将字符串转换为 double, std::strtod 函数：将字符串转换为 double
    }

    // expect(str, res): 期望 str, 返回 res
    // Expect that 'str' starts at the character that was just read. If it does, advance
    // the input and return res. If it does not, flag an error.
    // 期望 str 从刚刚读取的字符开始。如果是这样，前进输入并返回 res。如果不是，则标记一个错误。
    Json exepct(const string& expected, Json res) {
        assert(i != 0);
        i--;    // 解释：为什么要减 1，因为 get_next_token() 函数会自增 1
        if(str.compare(i, expected.length(), expected) == 0) {  // 解释：
            i += expected.length();
            return res;
        }else{
            return fail("parse error: expected " + expected + ", got " + str.substr(i, expected.length()), Json());
        }
    }

    // parse_json(int depth): 解析 JSON
    // Parse a JSON object: 解析 JSON 对象
    Json parse_json(int depth) {    // 解释 depth: 
        if(depth > max_depth) {
            return fail("exceeded maximum nesting depth");
        }

        char ch = get_next_token();
        if(failed) {
            return Json();
        }

        if(ch == '-' || (ch >= '0' && ch <= '9')) {
            i--;
            return parse_number();
        }

        if(ch == 't')
            return exepct("true", true);
        if(ch == 'f')
            return exepct("false", false);
        if(ch == 'n')
            return exepct("null", Json());
        if(ch == '"')
            return parse_string();
        if(ch == '{') {
            map<string, Json> data;
            ch = get_next_token();
            if(ch == '}') 
                return data;
            while(true) {
                if(ch != '"')
                    return fail("expected '\"' in object, got " + esc(ch));
                
                string key = parse_string();
                if(failed)
                    return Json();

                ch = get_next_token();
                if(ch != ':')
                    return fail("expected ':' in object, got " + esc(ch));
                
                data[move(key)] = parse_json(depth + 1);
                if(failed)
                    return Json();
                
                ch = get_next_token();
                if(ch == '}')
                    break;
                if(ch != ',')
                    return fail("expected ',' in object, got " + esc(ch));
                
                ch = get_next_token();
            }
            return data;
        }

        if(ch == '[') {
            vector<Json> data;
            ch = get_next_token();
            if(ch == ']')
                return data;
            while(true) {
                i--;
                data.push_back(parse_json(depth + 1));
                if(failed)
                    return Json();
                
                ch = get_next_token();
                if(ch == ']')
                    break;
                if(ch != ',')
                    return fail("expected ',' in list, got " + esc(ch));
                
                ch = get_next_token();
                (void)ch;
            }
            return data;
        }

        return fail("expected value, got " + esc(ch));
    }


};

} // namespace

Json Json::parse(const string& in, string& err, JsonParse strategy) {
    JsonParser parser{in, 0, err, false, strategy}; // {} 表示初始化
    Json result = parser.parse_json(0);

    // Check for any trailing garbage: 检查是否有任何尾随垃圾
    parser.consume_garbage();
    if(parser.failed)
        return Json();
    if(parser.i != in.size())
        return parser.fail("unexpected trailing " + esc(in[parser.i]));
    
    return result;
}

// Documented in json11.hpp: 在 json11.hpp 中有文档
vector<Json> Json::parse_multi(const string& in,
                                std::string::size_type &parser_stop_pos,
                                string& err,
                                JsonParse strategy) {
    JsonParser parser{in, 0, err, false, strategy};
    parser_stop_pos = 0;
    vector<Json> json_vec;
    while(parser.i != in.size() && !parser.failed) {
        json_vec.push_back(parser.parse_json(0));   // 0 表示 JSON 的深度
        if(parser.failed)
            break;

        // Check for another object: 检查另一个对象
        parser.consume_garbage();
        if(parser.failed)
            break;
        parser_stop_pos = parser.i;
    }
    return json_vec;
}

// Shape-checking: 形状检查
bool Json::has_shape(const shape& types, string& err) const {
    if(!is_object()) {
        err = "expected JSON object, got " + dump();
        return false;
    }

    const auto& obj_items = object_items();
    for(auto& item: types) {
        const auto it = obj_items.find(item.first); // 
        if(it == obj_items.cend() || it->second.type() != item.second) {
            err = "bad type for " + item.first + " in " + dump();
            return false;
        }
    }
    return true;
}

} // namespace json11