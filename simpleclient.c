#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int sockfd;
    int len;
    struct sockaddr_in address;
    int result;
    const int lenbuf=1024;
    char ch[lenbuf];
    char recvBuf[lenbuf];

    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(12010);
    len = sizeof(address);
    result = connect(sockfd, (struct sockaddr *)&address, len);

    printf("sockfd=%d\n",sockfd);  //debug

    if (result == -1)
    {
        perror("oops: client1");
        exit(1);
    }

    printf("Please input your get/post request:\n");

    read(STDIN_FILENO,ch,lenbuf);// 从标准输入流中读取一字符串

    ssize_t re=send(sockfd,ch,strlen(ch),0); //将ch所指的内存写入到sockfd所指的文件内，1个字节

    printf("Please input your desc:\n");
    read(STDIN_FILENO,ch,lenbuf);
    send(sockfd,ch,strlen(ch),0);

    ssize_t sz=recv(sockfd, recvBuf, lenbuf,0);

    printf("char from server = %s", recvBuf);

    close(sockfd);
    exit(0);
}
