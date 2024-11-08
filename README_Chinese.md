json11
------

json11 是一个适用于 C++11 的小型 JSON 库，提供 JSON 解析和序列化功能。

该库提供的核心对象是 json11::Json。一个 Json 对象可以表示任何 JSON 值：null、bool、数字（int 或 double）、字符串（std::string）、数组（std::vector）或对象（std::map）。

Json 对象像值一样行为。它们可以被赋值、复制、移动、比较相等或顺序等。此外，还有辅助方法 Json::dump 用于将 Json 序列化为字符串，以及静态方法 Json::parse 用于将 std::string 解析为 Json 对象。

使用 C++11 的新初始化语法创建 JSON 对象非常简单：

```cpp
Json my_json = Json::object {
    { "key1", "value1" },
    { "key2", false },
    { "key3", Json::array { 1, 2, 3 } },
};
std::string json_str = my_json.dump();
```

还有一些隐式构造函数允许标准和用户定义的类型自动转换为 JSON。例如：

```cpp
class Point {
public:
    int x;
    int y;
    Point(int x, int y) : x(x), y(y) {}
    Json to_json() const { return Json::array { x, y }; }
};

std::vector<Point> points = { { 1, 2 }, { 10, 20 }, { 100, 200 } };
std::string points_json = Json(points).dump();
```

可以查询和检查 JSON 值的内容：

```cpp
Json json = Json::array { Json::object { { "k", "v" } } };
std::string str = json[0]["k"].string_value();
```

更多文档请参见 json11.hpp。
