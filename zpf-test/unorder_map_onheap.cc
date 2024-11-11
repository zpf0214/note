#include <iostream>
#include <unordered_map>
#include <string>

// 函数声明
void populateMap(std::unordered_map<int, std::string>* mapPtr);
void printMap(const std::unordered_map<int, std::string>* mapPtr);

int main() {
    // 在堆上创建一个无序字典
    std::unordered_map<int, std::string>* myMap = new std::unordered_map<int, std::string>();

    // 填充无序字典
    populateMap(myMap);

    // 打印无序字典
    printMap(myMap);

    // 释放堆上的内存
    delete myMap;

    return 0;
}

// 填充无序字典的函数
void populateMap(std::unordered_map<int, std::string>* mapPtr) {
    if (mapPtr == nullptr) {
        std::cerr << "The map pointer is null.\n";
        return;
    }

    (*mapPtr)[1] = "one";
    (*mapPtr)[2] = "two";
    (*mapPtr)[3] = "three";
}

// 打印无序字典的函数
void printMap(const std::unordered_map<int, std::string>* mapPtr) {
    if (mapPtr == nullptr) {
        std::cerr << "The map pointer is null.\n";
        return;
    }

    // 遍历无序字典
    for (const auto& pair : *mapPtr) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << '\n';
    }
}
