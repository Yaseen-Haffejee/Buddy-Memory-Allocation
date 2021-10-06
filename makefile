
final: buddy.o test.o
	gcc buddy.o test.o -o final

buddy.o: buddy.c buddy.h
	gcc -c buddy.c

test.o: test.c buddy.h
	gcc -c test.c

clear: 
	rm *.o final