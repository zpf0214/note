#include <stdio.h>
#include <cstring>
#include <stdlib.h> // for exit and perror

int main() {
    // 定义文件路径
    const char *filepath = "example.txt";

    // 打开文件以进行读写。如果文件不存在，则创建它。
    // "w" 模式会清空文件内容（如果文件存在），并允许写入。
    // 如果你只想追加内容而不清空文件，可以使用 "a" 模式。
    FILE *file = fopen(filepath, "w+"); // w+ 表示读写模式，同时创建文件（如果不存在）

    if (file == NULL) {
        // 如果 fopen 返回 NULL，表示文件无法打开或创建
        perror("fopen 失败");
        return EXIT_FAILURE;
    }

    // 向文件中写入一些内容
    const char *message = "Hello, World!\n";
    size_t written = fwrite(message, sizeof(char), strlen(message), file);
    if (written != strlen(message)) {
        // 如果写入的字符数不等于字符串长度，说明发生了错误
        perror("fwrite 失败");
        fclose(file); // 确保关闭文件
        return EXIT_FAILURE;
    }

    // 将缓冲区的数据刷新到磁盘
    if (fflush(file) != 0) {
        perror("fflush 失败");
        fclose(file); // 确保关闭文件
        return EXIT_FAILURE;
    }

    // 重置文件位置指示器到文件开头，以便我们可以读取刚刚写入的内容

    // 从文件中读取内容
    char buffer[256];
    size_t read = fread(buffer, sizeof(char), sizeof(buffer) - 1, file);
    if (ferror(file)) {
        perror("fread 失败");
        fclose(file); // 确保关闭文件
        return EXIT_FAILURE;
    }

    // 确保字符串以 null 结尾
    buffer[read] = '\0';

    // 输出读取的内容到标准输出
    printf("读取的内容: %s", buffer);

    // 关闭文件
    if (fclose(file) != 0) {
        perror("fclose 失败");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
