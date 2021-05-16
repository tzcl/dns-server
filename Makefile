##
# COMP30023 Project 1
#

all: dns_svr phase1

dns_svr: dns_svr.c
	gcc -o dns_svr dns_svr.c -Wall

phase1: phase1.c packet.o
	gcc -o phase1 phase1.c packet.o -Wall

packet.o: packet.c packet.h
	gcc -c packet.c -Wall

.PHONY: clean

clean:
	rm -f phase1 dns_svr *.o

# end
