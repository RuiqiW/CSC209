PORT=59788
CFLAGS = -DPORT=\$(PORT) -g -std=c99 -Wall
# CFLAGS = -Wall -g

hcq_server: hcq_server.o hcq.o
	gcc $(CFLAGS) -o hcq_server hcq_server.o hcq.o

helpcentre: helpcentre.o hcq.o 
	gcc $(CFLAGS) -o helpcentre helpcentre.o hcq.o

hcq_server.o: hcq_server.c hcq.h
	gcc $(CFLAGS) -c hcq_server.c

helpcentre.o: helpcentre.c hcq.h
	gcc $(CFLAGS) -c helpcentre.c

hcq.o: hcq.c hcq.h
	gcc $(CFLAGS) -c hcq.c

clean: 
	rm helpcentre hcq_server *.o
