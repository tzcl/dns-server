##
# COMP30023 Project 1
#

dns_svr: dns_svr.c packet.o log.o list.o
	gcc -o dns_svr dns_svr.c packet.o log.o list.o -Wall

phase1: phase1.c packet.o log.o
	gcc -o phase1 phase1.c packet.o log.o -Wall

list.o: list.c list.h packet.h
	gcc -c list.c -Wall

packet.o: packet.c packet.h
	gcc -c packet.c -Wall

log.o: log.c log.h
	gcc -c log.c -Wall

.PHONY: clean

clean:
	rm -f phase1 dns_svr *.o

# end
