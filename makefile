PHONY:
	gcc -g main.c list.c -o wwm -lX11 -Wall -Wextra -O0
list:
	gcc -c list.c -Wall -Wextra
