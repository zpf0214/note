#include <iostream>
#include <optional>

int main() {
    std::optional<int> maybe_value;

    // 检查是否包含值
    if (!maybe_value.has_value()) {
        std::cout << "No value\n";
    }

    // 赋值
    maybe_value = 42;
    
    // 现在包含值
    if (maybe_value.has_value()) {
        std::cout << "Value: " << *maybe_value << "\n"; // 解引用获取值
    }

    // 或者使用 value() 方法，如果为空会抛出异常
    // std::cout << "Value: " << maybe_value.value() << "\n";

    // 使用 value_or 提供默认值
    std::cout << "Value or default: " << maybe_value.value_or(0) << "\n";

    return 0;
}
