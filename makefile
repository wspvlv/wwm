PHONY:
	gcc -g main.c list.c -o wwm -lX11 -Wall -Wextra -O2
list:
	gcc -c list.c -Wall -Wextra
