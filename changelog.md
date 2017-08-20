# 源码修改
我的系统时ubuntu16.04 LTS，需要做些修改才能运行。
> 按照README.md中的修改头文件进行了修改；
> 在源码的基础上添加了中文注释;
> 添加了调试代码，在适当的地方printf，输出代码运行情况;
'''
numchars = 25,and the buf = POST /color.cgi HTTP/1.1
method=POST
url=/color.cgi
path=htdocs/color.cgi
Open the path successfully
htdocs/color.cgi is a executable file
executing the file htdocs/color.cgi...
'''

> 修改源码中void accept_request(void *arg)函数的第一行，修改后结果如下：
'''
    //int client = (intptr_t)arg;  //修改前语句
    int client = *((int *)arg);   //修改后语句，将arg指针转换为int指针，再取指针内的值（socket描述符）
'''

> 修改了端口号，socket连接的端口号为12010

# perl安装
> 我的系统是ubuntu16.04 LTS，自带安装了perl，使用命令 'which perl' 可以查看perl的安装路径，我的是：
	/usr/bin/perl
   没有安装的话，安装：'sudo apt-get install perl'

# 修改cgi脚本

cgi脚本文件位于htdocs目录下，主要有color.cgi,check.cgi和index.html三个文件，其中cgi脚本是用perl写的，为了在linux下运行，做了一些修改：
> 用打开htdocs/color.cgi和htdocs/check.cgi发现，原文件的第一行包含perl的位置'#!/usr/local/bin/perl -Tw'和我系统中的不一样,于是改成：
'''
    #!/usr/bin/perl -Tw
'''

> 修改color.cgi和check.cgi文件的执行权限，进入到htdocs目录下执行：
    sudo chmod 764 color.cgi check.cgi

# makefile文件修改
见makefile文件，允许显示编译过程的warnings

# 运行说明
编译完成后，./httpd运行服务器端，然后根据ip地址和端口号（127.0.0.1，12010）在浏览器中访问服务器：
在浏览器中输入： 127.0.0.1:12010/color.cgi?color=red
回车便可看到页面执行效果。
client只用做调试，如果不是在本机上运行，需要将127.0.0.1修改为服务器端所在主机的ip。

