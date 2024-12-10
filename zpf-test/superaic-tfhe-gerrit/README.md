本项目是TFHE的C++实现。
# 编译
安装依赖
```
sudo apt install libboost-dev libssl-dev
```
如需要运行mtest，则还需要安装
```
sudo apt install valgrind
```
在代码根目录下执行
```
make
```
## 指定使用哪种方法进行多项式乘法
在makefile中通过环境变量CMAKE_MUL_OPTS来指定使用哪种多项式乘法，有以下几种可用的参数：
- -DMUL_TYPE="FFT"
- -DMUL_TYPE="NAIVE"
- -DMUL_TYPE="KARATSUB"

默认为-DMUL_TYPE="NAIVE"，可以在编译的时候指定使用哪种方法进行多项式乘法:
```
CMAKE_MUL_OPTS=-DMUL_TYPE="NAIVE" make
```
在编译的时候可以观察日志输出 ： using polynomial mul: 来确定到底使用了哪种多项式乘法。
# 测试
在代码根目录下执行
```
make test
```
查看测试输出
```
cat builddtests/Testing/Temporary/LastTest.log
```

## 访问设备文件没有权限
加入 superaic 用户组以后，重新登陆再执行。
```
sudo usermod -a -G superaic $(whoami)
```
