/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
#include <stdlib.h>
#define original_staticx


void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

// 응답을 해주는 함수
void doit(int fd){
  // 정적 파일인가 아닌가
  int is_static;
  //파일에 대한 정보를 가지고 있는 구조체
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  // rio(robust I/O(Rio)) 초기화
  Rio_readinitb(&rio, fd);
  // buf에서 client request 읽어들이기
  Rio_readlineb(&rio,buf,MAXLINE);
  printf("Request headers:\n");
  // request header 출력
  printf("%s",buf);
  // buf에 있는 데이터를 method,uri, version에 담는다
  sscanf(buf,"%s %s %s", method,uri,version);
  // method가 GET이 아니라면 error message 출력
  // problem 11.11을 위해 Head 추가
  if (strcasecmp(method,"GET") != 0 || strcasecmp(method,"HEAD") != 0){
    clienterror(fd,method,"501","Not implemented", "Tiny does not implement this error");
    return;  
  }
  read_requesthdrs(&rio);
  /* GET요청에서 URI(uniform resource identifier)를 출력*/
  // URI는 클라이언트가 서버에게 요청하는 리소스의 위치를 식별하는데 사용되는 부분
  is_static = parse_uri(uri,filename,cgiargs);
  // filename에 맞는 정보 조회를 하지 못했으면 error massage를 출력한ㄴ다
  if (stat(filename,&sbuf) <0 ){
    clienterror(fd,filename,"404","Not found","Tiny couldn't find this file");
    return;
  }
  // request file이 static contents이면 실행
  if(is_static){
    // file이 정규파일이 아니거나 사용자 읽기가 안되면 error message 출력
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd,filename, "403","Forbidden","Tiny couldn't read the file");
      return;
    }
    // 정적 파일을 응답한다. 정적파일 - 서버에 이미 저장되어 있는 파일
    serve_static(fd,filename,sbuf.st_size, method);
  }
  // request file이 다이나믹일 때 실행
  else {
    // file이 정규파일이 아니가나 사용자 읽기가 안되면 error message
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd,filename,"403","Forbidden","Tiny couldn't run the CGI program");
      return;
    }
     // 다이나믹 파일 반응
    serve_dynamic(fd,filename,cgiargs,method);
  }
}
// error 발생시 , client에게 보내기 위한 response (error message)
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];
  // response body 
  // html 헤더 생성
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body,"%s<p>%s: %s\r\n", body,longmsg,cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
  
  // response 쓰기
  // 버전, 에러번호 , 상태메시지
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum,shortmsg);
  Rio_writen(fd,buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd,buf, strlen(buf));
  sprintf(buf,"Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  // body 입력
  Rio_writen(fd, body, strlen(body));
}

// request header를 읽기 위한 함수
void read_requesthdrs(rio_t *rp)
{
 char buf[MAXLINE];
 Rio_readlineb(rp, buf, MAXLINE);
 // 빈 텍스트 줄이 아닐 때까지 읽기
 while(strcmp(buf, "\r\n")){
  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
 }
 return;
}

// uri parsing을 하여 static을 request할 경우 1, dynamic일 경우 0
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // parsing 결과, static file request -> uri에 cgi-bin이 포함이 되지 있지 않으면
  if (!strstr(uri, "cgi-bin")){
    // CGI 인수를 빈 문자열로 설정
    strcpy(cgiargs, "");
    // 파일 이름은 현재 디렉토리로 설정
    strcpy(filename,".");
    // URI를 파일 이름에 추가한다.
    strcat(filename, uri);

    // request에서 특정 static contents를 요구하지 않은 경우 (homepage 반환)
    // 즉 URI가 '/'로 끝나면 파일 이름에 'home.html'추가
    if (uri[strlen(uri)-1] == '/'){
      strcat(filename, "home.html");
    }
    return 1;
  }
  // parsing 했을 때, dynamic file request인 경우
  else{
    // uri 부분에서 filename과 args를 구분하는 ? 위치 찾기
    ptr = index(uri,'?');
    // ?가 있으면
    if (ptr) {
      // cgiargs에 인자 넣어주기
      strcpy(cgiargs, ptr+1);
      //포인터 ptr은 null처리
      *ptr='\0';
    }
    // ?가 없으면
    else {
      strcpy(cgiargs,"");
    }
    // filename에 uri담기
    strcpy(filename,".");
    strcat(filename,uri);
    return 0;
  }
}

#idef original_staticx


int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}
