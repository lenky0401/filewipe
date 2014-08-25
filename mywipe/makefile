release: filewipe.h filewipe.cpp
	g++ -fPIC -c -O2 filewipe.cpp -o filewipe.o
	ar -r libfilewipe.a filewipe.o
	g++ -shared -o libfilewipe.so filewipe.o

debug: filewipe.h filewipe.cpp
	g++ -D__DEBUG -g -O0 -o filewipe -Wall filewipe.cpp

clean:
	rm -f *.o filewipe libfilewipe.a libfilewipe.so
