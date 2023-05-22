#include "csapp.h"

void echo(int connfd); // 클라이언트로부터 받은 데이터를 그대로 되돌려 주는 함수

int main(int argc, char **argv)
{
    int listenfd, connfd; 
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2){  // 프로그램이 2개인지 아닌  확인
        fprintf(stderr, "usage %s <port>\n", argv[0]);
        exit(0);

    }
    listenfd = Open_listenfd(argv[1]); // argv[1] 인수에 저장된 포트 번호를 사용하여 서버 소켓을 열고 그 파일 설명자를 listenfd 변수에 저장
    while (1) {
        clientlen = sizeof(struct sockaddr_storage); // clientlen변수를  strcut sockaddr_storage 구조체의 크기로 설정
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
        // listenfd 파일 설명자를 사용하여 클라이언트의 연결을 수락하고 connfd에 저장
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE,0);
        // clientaddr 구조체에 저장된 클라이언트 IP주소와 포트 번호를 client_hostname배열과 client_port 배열에 저장
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        // 클라이언트 ip주소와 port번호 출력
        echo(connfd);
        // 클라이언트에게 받은 데이터를 그대로 되돌려줌 , 클라이언트와의 통신을 수행
        Close(connfd);
    }
    exit(0);
}