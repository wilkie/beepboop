blah: ymf262.c test.c a2m.c fmopl.c
	#gcc -o blah ymf262.c test.c -std=c99 -lm -lSDL -g
	g++ -o blah test.c dbopl.cpp -lm -lSDL -g
	#g++ -o blah fmopl.c test.c -lm -lSDL -g -DHAS_YM3812
