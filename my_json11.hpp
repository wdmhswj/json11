/* json11
 *
 * json11 是一个适用于 C++11 的小型 JSON 库，提供 JSON 解析和序列化功能。
 *
 * 该库提供的核心对象是 json11::Json。一个 Json 对象可以表示任何 JSON 值：null、bool、数字（int 或 double）、字符串（std::string）、数组（std::vector）或对象（std::map）。
 *
 * Json 对象像值一样行为：它们可以被赋值、复制、移动、比较相等或顺序等。此外，还有辅助方法 Json::dump 用于将 Json 序列化为字符串，以及静态方法 Json::parse 用于将 std::string 解析为 Json 对象。
 *
 * 内部，各种类型的 Json 对象由 JsonValue 类层次结构表示。
 *
 * 关于数字的说明 - JSON 规定了数字格式的语法，但没有规定其语义，因此一些 JSON 实现区分整数和浮点数，而有些则不区分。在 json11 中，我们选择后者。因为一些 JSON 实现（特别是 JavaScript 本身）将所有数字视为同一类型，区分这两种类型会导致在这些实现中往返传输时 JSON 被 *静默* 修改。这是危险的！为了避免这种风险，json11 内部将所有数字存储为 double，但也提供了整数辅助方法。
 *
 * 幸运的是，双精度 IEEE754（'double'）可以精确存储范围在 +/-2^53 内的任何整数，这包括大多数系统的每一个 'int'。（时间戳通常使用 int64 或 long long 以避免 Y2038K 问题；从某个纪元开始存储微秒的 double 在 +/- 275 年内都是精确的。）
 */

/* 版权所有 (c) 2013 Dropbox, Inc.
 *
 * 特此授予任何人获取本软件及随附文档文件（“软件”）的副本的权利，允许其不受限制地使用、复制、修改、合并、发布、分发、再许可和/或销售软件的副本，并允许向其提供软件的人这样做，前提是遵守以下条件：
 *
 * 上述版权声明和本许可声明应包含在所有副本或实质部分的软件中。
 *
 * 本软件按“原样”提供，不提供任何形式的明示或暗示的保证，包括但不限于适销性、特定用途适用性和非侵权性的保证。在任何情况下，作者或版权持有人都不对任何索赔、损害或其他责任负责，无论是在合同、侵权行为还是其他情况下，因软件或其使用或其他交易而引起或与之相关的任何索赔、损害或其他责任。
 */

#pragma once

#include <string>   // std::string
#include <vector>   // std::vector
#include <map>      // std::map
#include <memory>   // std::shared_ptr
#include <initializer_list> // std::initializer_list


// 预处理指令，用于确保与旧版本的 Microsoft Visual Studio（特别是 Visual Studio 2013 及更早版本）的兼容性
#ifdef _MSC_VER
    #if _MSC_VER <= 1800 // VS 2013
        #ifndef noexcept
            #define noexcept throw()
        #endif

        #ifndef snprintf
            #define snprintf _snprintf_s
        #endif
    #endif
#endif

namespace my_json11 {

// JsonParse: JSON 解析枚举，包括标准和注释
enum JsonParse {
    STANDARD, COMMENTS
};

class JsonValue;    // JsonValue 类的前置声明

class Json final {  // Json 类, final 修饰符表示该类不能被继承
public:
    // Type: Json 类型枚举，包括 空值、数字、布尔、字符串、数组、对象
    enum Type {
        NUL, NUMBER, BOOL, STRING, ARRAY, OBJECT
    };

    // Array and object typedefs: 数组和对象的类型定义
    typedef std::vector<Json> array;
    typedef std::map<std::string, Json> object;

    // Constructors for the various types of JSON value: 用于各种类型的 JSON 值的构造函数
    Json() noexcept;                // NUL, noexcept 表示不抛出异常
    Json(std::nullptr_t) noexcept;  // NUL, std::nullptr_t 表示空指针
    Json(double value);             // NUMBER
    Json(int value);                // NUMBER
    Json(bool value);               // BOOL
    Json(const std::string &value); // STRING
    Json(std::string &&value);      // STRING, && 表示右值引用, 可以实现移动语义, 用于避免不必要的内存拷贝
    Json(const char* value);        // STRING
    Json(const array &values);      // ARRAY
    Json(array&& values);           // ARRAY
    Json(const object& values);     // OBJECT
    Json(object&& values);          // OBJECT

    // Implicit constructor: anything with a to_json() function: 隐式构造函数：任何具有 to_json() 函数的对象
    template <class T, class = decltype(&T::to_json)>  // 模板类，用于判断 T 类是否有 to_json() 函数
    Json(const T& t) : Json(t.to_json()) {}           // 如果有 to_json() 函数，则调用该函数

    // Implicit constructor: map-like objects (std::map, std::unordered_map, etc): 隐式构造函数：类似于 map 的对象（std::map, std::unordered_map 等）
    // 这段代码定义了一个模板构造函数，用于从类似 map 的对象隐式构造 Json 对象。只有当 M 的键类型可以转换为 std::string，并且值类型可以转换为 Json 时，这个构造函数才会被实例化。
    template <class M, typename std::enable_if<
        std::is_constructible<std::string, decltype(std::declval<M>().begin()->first)>::value
        && std::is_constructible<Json, decltype(std::declval<M>().begin()->second)>::value,
        int>::type = 0>
    Json(const M& m) : Json(object(m.begin(), m.end())) {}  // 如果 M 类型是 map 类型，则调用该构造函数

    // Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc): 隐式构造函数：类似于 vector 的对象（std::list, std::vector, std::set 等）
    template <class V, typename std::enable_if<
        std::is_constructible<Json, decltype(*std::declval<V>().begin())>::value,
        int>::type = 0>
    Json(const V& v) : Json(array(v.begin(), v.end())) {}  // 如果 V 类型是 vector 类型，则调用该构造函数

    // This prevents Json(some_pointer) from accidentally producing a bool. Use Json(bool(some_pointer)) if that behavior is desired.
    // 这个构造函数防止 Json(some_pointer) 意外地产生一个 bool 值。如果需要这种行为，请使用 Json(bool(some_pointer))。
    Json(void*) = delete;  // 禁用 void* 类型的构造函数)

    // Accessors: 访问器
    Type type() const;  // 获取 Json 对象的类型

    bool is_null() const { return type() == NUL; }  // 判断 Json 对象是否为空值
    bool is_number() const { return type() == NUMBER; }
    bool is_bool() const { return type() == BOOL; }
    bool is_string() const { return type() == STRING; }
    bool is_array() const { return type() == ARRAY; }
    bool is_object() const { return type() == OBJECT; }

    // Return the enclosed value if this is a number, 0 otherwise.  // 如果是数字，则返回封装的值，否则返回 0
    // Note that json11 does not distingush between integer and non-integer numbers.
    // 注意，json11 不区分整数和非整数数字
    // number_value() and int_value() can both be applied to a NUMBER type Json object.
    // number_value() 和 int_value() 都可以应用于 NUMBER 类型的 Json 对象
    double number_value() const;
    int int_value() const;

    // Return the enclosed value if this is a boolean, false otherwise.
    bool bool_value() const;
    // Return the enclosed value if this is a string, "" otherwise.
    const std::string& string_value() const;
    // Return the enclosed std::vector if this is an array, or an empty vector otherwise.
    const array& array_items() const;
    // Return the enclosed std::map if this is an object, or an empty map otherwise.
    const object& object_items() const;

    // Return a reference to arr[i] if this is an array, Json() otherwise.
    const Json& operator[](size_t i) const;
    // Return a reference to obj[key] if this is an object, Json() otherwise.
    const Json& operator[](const std::string& key) const;

    // Serialize: 将 Json 对象序列化为字符串
    void dump(std::string& out) const;
    std::string dump() const {
        std::string out;
        dump(out);
        return out;
    }

    // Parse: If parse fails, return Json() and assign an error message to err.
    // 解析：如果解析失败，则返回 Json() 并将错误消息分配给 err
    static Json parse(const std::string& in, 
                      std::string& err,
                      JsonParse strategy = JsonParse::STANDARD);
    static Json parse(const char* in,
                      std::string& err,
                      JsonParse strategy = JsonParse::STANDARD) {
        if (in) {
            return parse(std::string(in), err, strategy);
        } else {
            err = "null input";
            return nullptr;
        }
    }

    // Parse multiple objects, concatenated or separated by whitespace.
    // 解析多个对象，这些对象可以是连接的，也可以是由空格分隔的
    static std::vector<Json> parse_multi(
        const std::string& in,
        std::string::size_type& parser_stop_pos,
        std::string& err,
        JsonParse strategy = JsonParse::STANDARD
    );
    
    static std::vector<Json> parse_multi(
        const std::string& in,
        std::string& err,
        JsonParse strategy = JsonParse::STANDARD
    ) {
        std::string::size_type parser_stop_pos;
        return parse_multi(in, parser_stop_pos, err, strategy);
    }

    // Comparison operators: 比较运算符
    bool operator==(const Json& rhs) const;
    bool operator<(const Json& rhs) const;
    bool operator!=(const Json& rhs) const { return !(*this == rhs); }
    bool operator<=(const Json& rhs) const { return !(rhs < *this); }
    bool operator>(const Json& rhs) const { return rhs < *this; }
    bool operator>=(const Json& rhs) const { return !(*this < rhs); }

    // has_shape(types, err)
    // Return true is this is a JSON object and, for each item in types, has a field of
    // the given type. If not, return false and set err to a description of the error.
    // 返回 true 如果这是一个 JSON 对象，并且对于 types 中的每个项目，都有一个给定类型的字段。如果没有，则返回 false 并将 err 设置为错误的描述。
    // has_shape 函数用于检查当前 JSON 对象是否具有特定的结构。它接受一个 shape 类型的参数 types，其中包含要检查的键值对列表。如果 JSON 对象具有这些键值对，则函数返回 true；否则，返回 false 并将错误描述存储在 err 中。
    typedef std::initializer_list<std::pair<std::string, Type>> shape;
    bool has_shape(const shape& types, std::string& err) const;

private:
    std::shared_ptr<JsonValue> m_ptr; // JsonValue 指针, 使用智能指针，避免内存泄漏

};

// Internal class hierarchy: 内部类层次结构
// JsonValue objects are not exposed to users of this API: JsonValue 对象不会向用户暴露此 API
class JsonValue {
protected:
    friend class Json;
    friend class JsonInt;
    friend class JsonDouble;
    virtual Json::Type type() const = 0;
    virtual bool equals(const JsonValue * other) const = 0;
    virtual bool less(const JsonValue * other) const = 0;
    virtual void dump(std::string &out) const = 0;
    virtual double number_value() const;
    virtual int int_value() const;
    virtual bool bool_value() const;
    virtual const std::string &string_value() const;
    virtual const Json::array &array_items() const;
    virtual const Json &operator[](size_t i) const;
    virtual const Json::object &object_items() const;
    virtual const Json &operator[](const std::string &key) const;
    virtual ~JsonValue() {} 
};

}