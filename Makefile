CC = gcc
CFLAGS = -lpthread
TARGET = encrypt

$(TARGET): encrypt-driver.c encrypt-module.c
	$(CC) $(CFLAGS) -o $(TARGET) encrypt-driver.c encrypt-module.c

clean:
	rm -f $(TARGET) *.o
