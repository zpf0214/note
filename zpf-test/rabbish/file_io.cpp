#include <iostream>
#include <fstream>
#include <string>

int main() {
    // 定义文件路径
    std::string path = "file.txt";

    // 创建或打开文件以进行写入（如果文件不存在，则会创建；如果存在，则会清空）
    std::ofstream file(path, std::ios::out | std::ios::trunc);

    // 检查文件是否成功打开
    if (!file.is_open()) {
        std::cerr << "无法打开或创建文件: " << path << std::endl;
        return 1; // 返回非零值表示错误
    }

    // 向文件中写入内容
    file << "Hello, World!" << std::endl;
    file << "这是第二个句子。" << std::endl;

    // 关闭文件
    file.close();

    // 检查是否成功写入
    if (file.good()) {
        std::cout << "内容已成功写入到文件: " << path << std::endl;
    } else {
        std::cerr << "写入文件时发生错误: " << path << std::endl;
    }

    return 0; // 返回零表示程序正常结束
}
