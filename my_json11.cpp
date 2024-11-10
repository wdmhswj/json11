

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

    
};
};
}; // namespace json11