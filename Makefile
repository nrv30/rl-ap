CFLAGS = -L C:\raylib\raylib\src -I C:\raylib\raylib\src
LFLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm

main: main.cpp
	g++ $(CFLAGS) main.cpp -o main $(LFLAGS)

test: test.c
	gcc $(CFLAGS) test.c -o test $(LFLAGS)

clean:
	del main.exe