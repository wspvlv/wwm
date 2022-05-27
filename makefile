cc = gcc main.c list.c -o wwm -lX11 -Wall -Wextra -Wno-deprecated-declarations -lm
PHONY: speed
debug:
	$(cc) -O0 -ggdb -DWWM_OPTIMIZE_SPEED=1 -DXSYNCHRONIZE=1
memory:
	$(cc) -Ofast -DWWM_OPTIMIZE_MEMORY=1
speed:
	$(cc) -Ofast -DWWM_OPTIMIZE_SPEED=1
size:
	$(cc) -Oz -DWWM_OPTIMIZE_MEMORY=1
