all: httpd client
#LIBS = -lpthread #-lsocket
httpd: httpd.c
	gcc -o $@ $<

client: simpleclient.c
	gcc -o $@ $<
clean:
	rm httpd
