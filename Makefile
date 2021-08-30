CC = gcc

waxcli:
	$(CC) src/main.c -o waxcli

clean:
	rm waxcli
