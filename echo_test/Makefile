CC = gcc
CFLAGS = -O2 -Wall -I.
# -O2 최적화 옵션, 컴파일러는 소스 코드 파일을 컴파일 할 때 성능 향상
# -Wall = 모든 경고를 표시하는 옵션, 컴파일러는 소스 코드 파일에 있는 모든 경고를 표시
# -I. 현재 디렉토리를 include경로에 추가하는 옵션으로 , 현재 디렉토리에 있는 헤더 파일을 include경로로 추가.
LIB = -lpthread
# LIB링커옵션을 지정하는 변수, 지정된 변수는 컴파일러가 소스 코드 파일을 링크할 때 사용된다.
# 링커는 프로그램을 실행 가능한 바이너리 형태로 만들 때, 여러 개의 오브젝트 파일을 하나의 실행 파일로 결합하는 역할
all: echo_server echo_client
# all규칙 : echo_server와 echo_client를 빌드한다.
echo_server: echo_server.c echo.o csapp.o
	$(CC) $(CFLAGS) -o echo_server echo_server.c echo.o csapp.o $(LIB)
# echo_sever.c echo.o csapp.o를 컴파일 하여 echo_server실행 파일을 생성한다.
echo.o: echo.c
	$(CC) $(CFLAGS) -c echo.c
# echo.c를 컴파일 하여 echo.o 오브젝트 파일을 생성한다.
echo_client: echo_client.c csapp.o
	$(CC) $(CFLAGS) -o echo_client echo_client.c csapp.o $(LIB)
# echo_client .c, csapp.o를 컴파일하여 echo_client실행 파일을 생성한다.
csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c
# csapp.c를 컴파일하여 csapp.o 오브젝트 파일을 생성한다.
clean:
	rm -f *.o echo *~
	rm -f echo_server echo_client
