all: psclient psserver libstringmap.so
psclient: psclient.c
	gcc -pthread -pedantic -Wall -std=gnu99 -I/local/courses/csse2310/include -L/local/courses/csse2310/lib -lcsse2310a4 -lcsse2310a3 -o $@ $<

psserver: psserver.c
	gcc -pthread -pedantic -Wall -std=gnu99 -I/local/courses/csse2310/include -L/local/courses/csse2310/lib -lstringmap -lcsse2310a4 -lcsse2310a3 -o $@ $<


CC=gcc
LIBCFLAGS=-fPIC -Wall -pedantic -std=gnu99
# Turn stringmap.c into stringmap.o
stringmap.o: stringmap.c
	$(CC) $(LIBCFLAGS) -c $<

# Turn stringmap.o into shared library libstringmap.so
libstringmap.so: stringmap.o
	$(CC) -shared -o $@ stringmap.o

clean:
	rm -f psclient.c psserver.c libstringmap.so
