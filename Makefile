libcoro.dylib: coro.h coro.c
	clang -Wall -I. -dynamiclib -install_name @rpath/libcoro.dylib -arch i386 -arch x86_64 -o $@ coro.c

tests: libcoro.dylib tests.c
	clang -I. -L. -rpath @executable_path/. -lcoro -o $@ tests.c

all: libcoro.dylib tests

clean:
	rm -f tests libcoro.dylib
