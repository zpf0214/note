#include <iostream>

using namespace std;

int main(){
    string str = "a";
    cout << str[0] << endl;

    { //zpf char -> int32_t
        int32_t num = 97;
        char ch = static_cast<char>(num);
        cout << "The character for ASCII value: "
            << num << " is: " << ch << endl;
    }

    {
        char ch = 97;
        cout << ch << endl;
    }

    {
        char ch = 'a';
        int32_t num = static_cast<int32_t>(ch);
        cout << num << endl;
    }
    //zpf 经过以上测试我们知道字符和数字没有任何区别，我们需要做的事情也没有任何区别，
    //接下来我们试试字符串

    {
        string a = "abc";
        for (char ch: a){
            cout << static_cast<int32_t>(ch) << ", ";
        }
        cout << endl;
    }

    {
        //zpf 向字符串添加字符
        int32_t num = 97;
        string s = "";
        {
            for(int i=0; i<3; i++){
                s.push_back(static_cast<char>(++num));
            }
        }
        cout << s << endl;
    }

    return 0;
}
