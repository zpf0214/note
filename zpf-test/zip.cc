#include <iostream>
#include <vector>
#include <ranges>
#include <tuple>

int main() {
    std::vector<int> v1 = {1, 2, 3, 4};
    std::vector<char> v2 = {'a', 'b', 'c', 'd'};

    // 使用 C++20 的 ranges 和 views::zip
    for (auto [num, ch] : std::views::zip(v1, v2)) {
        std::cout << num << " - " << ch << '\n';
    }

    return 0;
}
