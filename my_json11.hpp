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

#include <string>
#include <vector>   // std::vector
#include <map>
#include <memory>
#include <initializer_list>


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

enum JsonParse {
    STANDARD, COMMENTS
};

class JsonValue;    // JsonValue 类的前置声明

class Json final {  // Json 类, final 修饰符表示该类不能被继承
    // Type: Json 类型枚举，包括 空值、数字、布尔、字符串、数组、对象
    enum Type {
        NUL, NUMBER, BOOL, STRING, ARRAY, OBJECT
    };

    // Array and object typedefs: 数组和对象的类型定义
    typedef std::vector<Json> array;
    typedef std::map<std::string, Json> object;

};
}