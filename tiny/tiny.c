/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"



void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

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
  if(strcasecmp(method, "GET")){
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);
  /* GET요청에서 URI(uniform resource identifier)를 출력*/
  // URI는 클라이언트가 서버에게 요청하는 리소스의 위치를 식별하는데 사용되는 부분
  is_static = parse_uri(uri,filename,cgiargs);
  // filename에 맞는 정보 조회를 하지 못했으면 error massage를 출력한다
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
    serve_static(fd,filename,sbuf.st_size);
  }
  // request file이 다이나믹일 때 실행
  else {
    // file이 정규파일이 아니가나 사용자 읽기가 안되면 error message
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd,filename,"403","Forbidden","Tiny couldn't run the CGI program");
      return;
    }
     // 다이나믹 파일 반응
    serve_dynamic(fd,filename,cgiargs);
  }
}
// error 발생시 , client에게 보내기 위한 response (error message)
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];
  // response body 
  // html 헤더 생성
  sprintf(body, "<html><title>Tiny Error</title>");
  // HTML 응답 바디 생성
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
/*
parse_uri 
정적 컨텐츠를 위한 홈 디렉토리를 설정하고 
실행파일의 홈디렉토리를 설정한다.
*/

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
/* serve_static
지역 파일의 내용을 포함하고 있는 본체를 갖는 HTTP 응답을 보낸다.
먼저 파일 이름의 접미어 부분을 검사해서 파일 타입을 결정하고
클라이언트에 응답 중과 응답 헤더를 보낸다.
*/
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
 /*
 serve_static 함수는 정수형 fd, 문자열 포인터 filename,정수형 filesize 세 개의 매개변수를 받는다. 
 fd - 클라이언트와의 연결에 사용되는 파일 디스크립터
 filename - 전송할 정적 파일의 경로와 이름
 filesize - 전송할 파일의 크기
 */
 // HTTP 응답 헤더를 생성하여 buf에 저장한다.
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf,"%sServer: Tiny Web Server\r\n",buf);
  sprintf(buf,"%sConnection: close\r\n", buf);
  sprintf(buf,"%sContent-length: %d\r\n",buf,filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n",buf,filetype);
  // 생성된 응답 헤더를 클라이언트에게 전송한다
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s",buf);
// 요청한 파일을 읽기 전용으로 열고, 파일의 내용을 srcp에 매핑
  srcfd = Open(filename, O_RDONLY,0);
  srcp = Mmap(0,filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  // 매핑된 파일의 내용을 클라이언트에게 전송한다
  Rio_writen(fd,srcp,filesize);
  // 소켓 fd에 srcp의 내용을 전송
  // 매핑된 메모리를 해제
  Munmap(srcp,filesize);
  // Munmap함수를 사용하여 srcp에 할당된 메모리를 해제

  /* 11.9 homework
    정적컨텐츠를 처리 할 때, 요청한 파일을 mmap과 rio_readn대신 malloc, rio_readn, rio_writen을 사용해서 연결식별자에게 복사
    // 요청한 파일을 읽기 전용으로 열고, 파일의 내용을 srcp에 복사한다.
    srcfd = Open(filename, O_RDONLY,0); 이후 부터 시작된다.
    char *srcp = malloc(filesize);
    rio_readn(srcfd, srcp, filesize);
    Close(srcfd);

    // srcp의 내용을 클라이언트에게 전달
    Rio_writen(fd,srcp, filesize);

    // 할당된 메모리를 해제한다.
    free(srcp);
    
  
  */
}
/* get_filetype 
파일의 컨텐츠 타입을 확인하는 함수
*/
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");  // filename에 .html이 포함되어 있다면 text/html을 복사합니다.   
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");  // filename에 .gif가 포함되어 있다면 ,filetype에 image/gif를 복사한다
  else if (strstr(filename, ".png")) 
    strcpy(filetype, "image/png"); // filename에 .png가 포함되어 있다면 ,filetype에 image/png을 복사한다
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg"); // filename에 .jpg가 포함되어 있다면 ,filetype에 image/jpeg을 복사한다
  
  else if(strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4"); // 비디오 파일을 읽을 수 있게 URI parese시 처리할 수 있는 filetype에 mp4추가
  else
    strcpy(filetype, "text/plain"); // filename에 .gif가 포함되어 있다면 ,filetype에 text/plain을 복사한다
}
/*
serve_dynamic 
동적 컨텐츠를 자식의 컨텍스트에서 실행할 수 있도록 해주는 함수다
*/
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] ={ NULL };
  
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd,buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd,buf,strlen(buf));

  if(Fork() == 0){
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
}
