基于[webbench](http://home.tiscali.cz/~cz210552/webbench.html)基础上，用c++重构

重构后，可读性更强

- 将命令参数解析封装为WebbenchOption 类
- 将fork后子进程父进程处理过程，重构为bench_child bench_father两个函数
- 通过传参方式，将原来大多全局变量改为局部变量

使用改写源码说明

- 继承原软件版权协议GPL