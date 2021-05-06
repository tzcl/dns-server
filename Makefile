##
# COMP30023 Project 1
#

dns_svr: dns_svr.c
	gcc -o dns_svr dns_svr.c -Wall

.PHONY: clean

clean:
	rm -f dns_svr *.o

# end
