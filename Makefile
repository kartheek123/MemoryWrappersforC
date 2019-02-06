CC = gcc

libwrappers.so: wrappers.c
				$(CC) -fPIC -shared  -o libwrappers.so wrappers.c -ldl
clean: 
		rm *.so