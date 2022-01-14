# KITE
## 如何编译
```bash
#-c只编译不链接，生成文件cppfuncs.o
#-std=c++11指定C++11标准
#-fpic告诉编译器生成位置无关代码(position-independent code)
g++ -c cppfuncs.cpp -std=c++11 -fpic
#ar命令将目标文件(*.o)打包成静态链接库(.a)，生成文件libcppfuncs.a
ar -crv libcppfuncs.a cppfuncs.o
make
```