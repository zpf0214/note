#include<iterator>
#include<ranges>
#include<iostream>
#include<vector>
#include<list>
#include<string>

using namespace std;

template<typename It>
concept IterableIterator = requires(It it) {
    *it;
    ++it;
    {it != it} -> std::convertible_to<bool>;
};

template<typename Container>
concept IterableContainer = requires(Container c) {
    typename std::iterator_traits<decltype(std::begin(c))>::iterator_category;
    {std::begin(c)} -> IterableIterator;
    {std::end(c)} -> IterableIterator;
};

template<typename Func, IterableContainer Rng>
auto fmap(Func func, Rng&& rng) {
    return std::ranges::transform_view(std::forward<Rng>(rng), func);
}

int main() {
    vector<int> vec = {1, 2, 3, 4};
    auto doubleVec = fmap(
            [](int x){ return x*2; }, 
            vec);

    char8_t str[] = u8"test";
    cout << str << endl;
    //for(const auto s: str) {
    //    cout << s << ;

    //}
    return 0;
}
