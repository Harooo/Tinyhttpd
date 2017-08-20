/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 *  1) Comment out the #include <pthread.h> line.
 *  2) Comment out the line that defines the variable newthread.
 *  3) Comment out the two lines that run pthread_create().
 *  4) Uncomment the line that runs accept_request().
 *  5) Remove -lsocket from the Makefile.
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
//#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdint.h>

#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"
#define STDIN   0
#define STDOUT  1
#define STDERR  2

void accept_request(void *);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
int startup(u_short *);
void unimplemented(int);

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(void *arg)  //step3
{
    //int client = (intptr_t)arg;  //修改前语句
    int client = *((int *)arg);   //修改后语句，将arg指针转换为int指针，再取指针内的值（socket描述符）
    char buf[1024];
    size_t numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    struct stat st;
    int cgi = 0;      /* becomes true if server decides this is a CGI
                       * program */
    char *query_string = NULL;

    numchars = get_line(client, buf, sizeof(buf)); /*得到请求的第一行*/ //读取第一行，返回值是该行字符数。
    printf("numchars = %d,and the buf = %s",numchars,buf);
    i = 0; j = 0;
    while (!ISspace(buf[i]) && (i < sizeof(method) - 1))  /*把客户端的请求方法存到 method 数组 去掉了空格符 */
    {
        method[i] = buf[i];
        i++;
    }
    j=i;
    method[i] = '\0';

    printf("method=%s\n",method);
    /*如果既不是 GET 又不是 POST 则无法处理 */
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))  //strcasecmp(s1,s2)会比较字符串，忽略大小写差异，相同返回0，s1大返回大于零的值，s1小于s2返回小于零的值。
    {
        unimplemented(client);
        return;
    }

     /* POST 的时候开启 cgi */
    if (strcasecmp(method, "POST") == 0)
        cgi = 1;

    /*读取 url 地址*/
    i = 0;
    while (ISspace(buf[j]) && (j < numchars))
        j++;
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < numchars))
    {
        /*存下 url */
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';  //URL

    printf("url=%s\n",url);

     /*处理 GET 方法*/
    if (strcasecmp(method, "GET") == 0)
    {
        /* 待处理请求为 url */
        query_string = url;  //将数组名赋给char指针query_string
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;

        /* GET 方法特点，? 后面为参数*/
        if (*query_string == '?')
        {
            /*开启 cgi */
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }

        printf("query_string=%s\n",query_string);
    }

     /*格式化 url 到 path 数组，html 文件都在 htdocs 中*/
    sprintf(path, "htdocs%s", url);
    /*默认情况为 index.html */
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");  //将index.html添加到path末尾

    printf("path=%s\n",path);
    /*根据路径找到对应文件 */
    if (stat(path, &st) == -1) {  //将path所指的文件的状态复制到st所指的结构中，执行成功返回0，失败返回-1
        /*把所有 headers 的信息都丢弃*/
        printf("Open the path failed\n");
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
        /*回应客户端找不到*/
        not_found(client);
    }
    else
    {
        printf("Open the path successfully\n");
        /*如果是个目录，则默认使用该目录下 index.html 文件*/
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path, "/index.html");
        if ((st.st_mode & S_IXUSR) ||
                (st.st_mode & S_IXGRP) ||
                (st.st_mode & S_IXOTH)    )
        {
            printf("%s is a executable file\n",path);
            cgi = 1;
        }

        /*不是 cgi,直接把服务器文件返回，否则执行 cgi */
        if (!cgi)
            serve_file(client, path);
        else
        {
            printf("executing the file %s...\n",path);
            execute_cgi(client, path, method, query_string);
        }
    }

     /*断开与客户端的连接（HTTP 特点：无连接）*/
    close(client);
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
    char buf[1024];

    /*回应客户端错误的 HTTP 请求 */
    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, FILE *resource)
{
    char buf[1024];

    fgets(buf, sizeof(buf), resource);  //resource读取到buf中
    while (!feof(resource)) //检查文件流是否读到了文件尾，如果到文件尾返回非零值，其他情况返回0
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);//继续读取resource中的内容
    }
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
    char buf[1024];

    /* 回应客户端 cgi 无法执行*/
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
    /*出错信息处理 */
    perror(sc);
    exit(1);
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(int client, const char *path,
        const char *method, const char *query_string)   //step4
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A'; buf[1] = '\0';
    if (strcasecmp(method, "GET") == 0)  //strcasecmp比较两个字符串，相同则返回0
         /*把所有的 HTTP header 读取并丢弃*/
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        {
            numchars = get_line(client, buf, sizeof(buf));
        }
    else if (strcasecmp(method, "POST") == 0) /*POST*/
    {
         /* 对 POST 的 HTTP 请求中找出 content_length */
        numchars = get_line(client, buf, sizeof(buf));
        while ((numchars > 0) && strcmp("\n", buf))
        {
            /*利用 \0 进行分隔 */
            buf[15] = '\0';
            /* HTTP 请求的特点*/
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16]));  //将字符串转换成整型数
            numchars = get_line(client, buf, sizeof(buf));
        }
        /*没有找到 content_length */
        if (content_length == -1) { //content_length默认值是-1
            /*错误请求*/
            bad_request(client);
            return;
        }
    }
    else/*HEAD or other*/
    {
    }

    /* 建立管道*/
    if (pipe(cgi_output) < 0) {
        /*错误处理*/
        cannot_execute(client);
        return;
    }
    /*建立管道*/
    if (pipe(cgi_input) < 0) {  //pipe  位于unistd.h头文件中，建立管道，并将文件描述词由参数cgi_output返回。成功返回0，否则返回-1
        /*错误处理*/
        cannot_execute(client);
        return;
    }

    if ( (pid = fork()) < 0 ) { //fork()分叉函数，开辟一个子进程
        /*错误处理*/
        cannot_execute(client);
        return;
    }

     /* 正确，HTTP 状态码 200 */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    if (pid == 0)  /* child: CGI script */  //子进程
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];
        /* 把 STDOUT 重定向到 cgi_output 的写入端 */
        int dn1 = dup2(cgi_output[1], STDOUT);

        /* 把 STDIN 重定向到 cgi_input 的读取端 */
        int dn2 = dup2(cgi_input[0], STDIN);

        /* 关闭 cgi_input 的写入端 和 cgi_output 的读取端 */
        close(cgi_output[0]);
        close(cgi_input[1]);
        /*设置 request_method 的环境变量*/
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0) {
            /*设置 query_string 的环境变量*/
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else {   /* POST */
            /*设置 content_length 的环境变量*/
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        /*用 execl 运行 cgi 程序*/
        execl(path, NULL);
        exit(0);
    } else {    /* parent */
        /* 关闭 cgi_input 的读取端 和 cgi_output 的写入端 */
        close(cgi_output[1]);
        close(cgi_input[0]);
        if (strcasecmp(method, "POST") == 0)
             /*接收 POST 过来的数据*/
            for (i = 0; i < content_length; i++) {
                recv(client, &c, 1, 0);
                /*把 POST 数据写入 cgi_input，现在重定向到 STDIN */
                write(cgi_input[1], &c, 1);
            }

        /*读取 cgi_output 的管道输出到客户端，该管道输入是 STDOUT */
        while (read(cgi_output[0], &c, 1) > 0)
            send(client, &c, 1, 0);

        /*关闭管道*/
        close(cgi_output[0]);
        close(cgi_input[1]);
        /*等待子进程*/
        waitpid(pid, &status, 0);
    }
}

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size) //从socket中接收数据，存到buf中，size是buf的大小，返回值是接收的字符数。
{
    int i = 0;
    char c = '\0';
    int n;

    /*把终止条件统一为 \n 换行符，标准化 buf 数组*/
    while ((i < size - 1) && (c != '\n'))
    {
        /*一次仅接收一个字节*/  //接收成功则返回接收的字符数，失败返回-1.

        n = recv(sock, &c, 1, 0);
        //DEBUG printf("%02X\n", c);
        if (n > 0)
        {
            /*收到 \r 则继续接收下个字节，因为换行符可能是 \r\n */
            if (c == '\r')
            {
                /*使用 MSG_PEEK 标志使下一次读取依然可以得到这次读取的内容，可认为接收窗口不滑动*/
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */

                /*但如果是换行符则把它吸收掉*/
                if ((n > 0) && (c == '\n'))  //接收到一个字符且接收到的字符为换行符
                    recv(sock, &c, 1, 0);
                else  //未接收到字符
                    c = '\n';
            }
            /*存到缓冲区*/
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';  //结束符'\0'

    /*返回 buf 数组大小*/
    return(i);
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename;  /* could use filename to determine file type */

    /*正常的 HTTP header */
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    /*服务器信息*/
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
    char buf[1024];

    /* 404 页面 */
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    /*服务器信息*/
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    /*读取并丢弃 header */
    buf[0] = 'A'; buf[1] = '\0';
    while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        numchars = get_line(client, buf, sizeof(buf));

    /*打开 sever 的文件*/
    resource = fopen(filename, "r");
    if (resource == NULL)
        not_found(client);
    else
    {
        /*写 HTTP header */
        headers(client, filename);
        /*复制文件*/
        cat(client, resource);
    }
    fclose(resource);
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(u_short *port)   //服务器启动，在指定端口port或随机选取端口绑定httpd服务
{
    int httpd = 0;
    int on = 1;
    struct sockaddr_in name;  //定义internet环境下socket套接字地址

    httpd = socket(PF_INET, SOCK_STREAM, 0);  //PF:Protocol Family,domain,type和protocol，返回socket套接字描述符。
    if (httpd == -1)  //如果建立socket出错，则返回-1.
        error_die("socket");  //自定义函数error_die,使用perror打印上一函数错误原因。
    memset(&name, 0, sizeof(name)); //先清空name
    name.sin_family = AF_INET; //协议簇 AF:Address Family
    name.sin_port = htons(*port);//htons函数的作用是将端口号由主机字节序转换为网络字节序的整数值。针对的是两个字节16位的(host to net)
    name.sin_addr.s_addr = htonl(INADDR_ANY); //作用域htons相同，但htonl针对的是32位的，用于转换地址。与htonl()和htons()作用相反的两个函数是：ntohl()和ntohs()。
    if ((setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)  //设置socket的状态，成功返回0，有错误返回-1.
    {
        error_die("setsockopt failed");
    }
    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0) //bind用来设置给httpd的socket绑定一个名称，此名称由地址参数name指向一sockaddr结构。成功返回0，出错返回-1.
        error_die("bind"); //出错
    if (*port == 0)  /* if dynamically allocating a port，如果当前指定端口是 0，则动态随机分配一个端口*/
    {
        socklen_t namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)  //调用getsockname（）来返回分配给此连接的本地IP地址和端口,存放在name中。
            error_die("getsockname");
        *port = ntohs(name.sin_port); //ntohs()用来将参数指定的16位netshort转换成主机字符顺序，返回对应的主机字符顺序。
    }
    if (listen(httpd, 5) < 0)   //监听 通常listen会在socket(),bind()之后调用。监听成功返回0，失败返回-1.
        error_die("listen");
    return(httpd);
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
    char buf[1024];

    printf("This is unimplemented.\n");
    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n"); //根据第二个参数的格式来转换数据，并将结果复制到buf中。
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/

int main(void)  //step1
{
    int server_sock = -1;
    u_short port = 12010; //指定port， u_short就是unsigned short
    int client_sock = -1;
    struct sockaddr_in client_name;
    socklen_t  client_name_len = sizeof(client_name);
    //pthread_t newthread;  //define variable 'newthread'  定义一个线程标识符

    server_sock = startup(&port);  //step2:初始化httpd服务，包括建立套接字，绑定端口，进行监听等。返回参数为socket描述符（整型）
    printf("sever sock:%d\n",server_sock);

    printf("httpd running on port %d\n", port);
    while (1)
    {
        /*accept()函数接收参数server_sock的socket连线,一个accept可以新建一条与服务器socket的连线。
        当有连线进来时，accept会返回一个新的socket处理代码.
        */
        client_sock = accept(server_sock,
                (struct sockaddr *)&client_name,  //连接成功时，client_name会被系统填入远程主机的地址数据。
                &client_name_len);  //被填入地址结构长度
        if (client_sock == -1)  //accept()成功返回新建立的socket描述符，失败返回-1
            error_die("accept\n");

        accept_request(&client_sock);      //自定义accept_request()函数

        //pthread_create是类Unix操作系统（Unix、Linux、Mac OS X等）的创建线程的函数。
        //if (pthread_create(&newthread , NULL, (void *)accept_request, (void *)(intptr_t)client_sock) != 0)
          //  perror("pthread_create");
    }

    close(server_sock);

    return(0);
}
