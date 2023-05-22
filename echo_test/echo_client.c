#include "csapp.h"
// 클라이언트가 전송하는 모든 데이터를 표준 출력으로 출력
int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3){ // 프로그램의 인수가 3인지 확인 , 3개가 아니라면 종료: 프로그램의 실행파일명, 서버의 호스트명, 서버의 포트명
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1]; // IP주소를 host 변수에 저장
    port = argv[2]; // port번호를 port 변수에 저장

    clientfd = Open_clientfd(host, port); // 클라이언트 소켓을 열고 clientfd 변수에 저장 + 서버에 연결
    Rio_readinitb(&rio, clientfd); // 읽기 연산을 수행하기 위해 rio구조체 초기화, rio 구조체는 입력/출력 버퍼를 관리

    while (Fgets(buf,MAXLINE,stdin) != NULL) {
        Rio_writen(clientfd, buf,strlen(buf)); // buf배열에 저장된 데이터를 클라이언트로 전송
        rio_readlineb(&rio, buf, MAXLINE); // 클라이언트로부터 한 줄씩 읽어들인다.
        Fputs(buf,stdout); // 표준 출력으로 출력한다

    } // 표준 입력에서 한 줄씩 읽어와서 buf에 저장하고 입력이 없을 때까지 반복
    Close(clientfd);
    exit(0);

}