
### Introduction

隐藏式动态加载一个apk并运行，dex加载使用的是内存dex加载，so加载使用的是自定义linker,从内存中加载so。这可以防止so maps路径检测，so遍历。

当前项目版本，理论上来说可以支持任何改动不太大的android 系统的soinfo

它是一个黑客工具，也可以是一个安全项目，他的主要作用就是隐藏



### support aosp
+ android11
+ android12
+ android13


### deficiencies
+ so加载，使用的是apk中的so自动加载，System.load这些主动加载函数不必调用（如果你调用了，会找不到了so，路径问题不多说）
+ so单例，同一个apk中的so不能互相依赖，比如我们经常使用liglog.so，动态库打到apk中是不行的，可以看看demo中我是如何处理的。
+ 只能在jni_onload中使用动态注册的java native函数，不支持静态java native函数，静态java native函数需要一个解析系统，这部分兼容没搞
+ so只能单独使用，不会加入到系统库里，如果想要外部使用，或者联动，需要在自定义一个linker so链表，目前没做


### expand

目前soinfo兼容有两种方案，第一种动态定位重新赋值soinfo,第二种，不断地进行so版本更新，将soinfo结构体写死
第一种方案兼容好，更优雅，但是可能会慢，第二种方案更快，但是需要一直兼容，我觉得不会有人选择第二种


### test
直接安装app测试hideapk在你的手机系统上是否支持
需要安装fridaload这个测试app，让hideapk的测试功能进行加载，写死的


# Future

+ 更为强大的so防检测功能，将so完全打碎，使任何so检测，符号检测，so格式扫描，so特征检测完全无效
+ 落地生根，寻找适合的路径，并进行伪装

