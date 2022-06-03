cc = gcc main.c list.c -o wwm -lX11 -Wall -Wextra -lm -fprofile
PHONY: speed
debug:
	$(cc) -O0 -ggdb -DWWM_OPTIMIZE_SPEED=1 -DXSYNCHRONIZE
memory:
	$(cc) -Ofast -DWWM_OPTIMIZE_MEMORY=1
speed:
	$(cc) -Ofast -DWWM_OPTIMIZE_SPEED=1
size:
	$(cc) -Oz -DWWM_OPTIMIZE_MEMORY=1
