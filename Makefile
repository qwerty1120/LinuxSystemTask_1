CC = gcc

HEADER = ssu_header
BACKUP = ssu_backup
HELP = ssu_help

$(BACKUP) : $(BACKUP).o $(HELP).o
	$(CC) -o $(BACKUP) $(BACKUP).o $(HELP).o -lcrypto -g

$(BACKUP).o : $(HEADER).h $(BACKUP).c
	$(CC) -c -o $@ $(BACKUP).c -lcrypto -g

$(HELP).o : $(HELP).c
	$(CC) -c -o $@ $(HELP).c -lcrypto -g


clean :
	rm -rf $(BACKUP)
	rm -rf *.o

all : $(BACKUP)
