CCOMPILER=clang++
CCFLAGS=-Os -fno-exceptions -ffunction-sections -fdata-sections -fno-rtti -fno-stack-protector -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables -s

build:
	cls && $(CCOMPILER) $(CCFLAGS) sizeof.c -o sizeof.exe
	strip -g -s -S -d --strip-debug --merge-notes --strip-all --strip-dwo --discard-locals sizeof.exe

all:
	gcc $(CCFLAGS) sizeof.c -o gccCx64.exe
	g++ $(CCFLAGS) sizeof.c -o g++C++x64.exe
	clang $(CCFLAGS) sizeof.c -o clangCx64.exe
	clang++ $(CCFLAGS) sizeof.c -o clangC++x64.exe

	clang $(CCFLAGS) -m32 sizeof.c -o clangCx86.exe
	clang++ $(CCFLAGS) -m32 sizeof.c -o clangC++x86.exe

	strip -g -s -S -d --strip-debug --merge-notes --strip-all --strip-dwo --discard-locals gccCx64.exe
	strip -g -s -S -d --strip-debug --merge-notes --strip-all --strip-dwo --discard-locals g++C++x64.exe
	strip -g -s -S -d --strip-debug --merge-notes --strip-all --strip-dwo --discard-locals clangCx64.exe
	strip -g -s -S -d --strip-debug --merge-notes --strip-all --strip-dwo --discard-locals clangC++x64.exe
	strip -g -s -S -d --strip-debug --merge-notes --strip-all --strip-dwo --discard-locals clangCx86.exe
	strip -g -s -S -d --strip-debug --merge-notes --strip-all --strip-dwo --discard-locals clangC++x86.exe

gcc86:
	gcc $(CCFLAGS) sizeof.c -o gccCx86.exe
	g++ $(CCFLAGS) sizeof.c -o g++C++x86.exe
	strip -g -s -S -d --strip-debug --merge-notes --strip-all --strip-dwo --discard-locals gccCx86.exe
	strip -g -s -S -d --strip-debug --merge-notes --strip-all --strip-dwo --discard-locals g++C++x86.exe

msvcC:
	rename 1.exe msvcCx86.exe
	rename 2.exe msvcCx64.exe

msvcCpp:
	rename 1.exe msvcC++x86.exe
	rename 2.exe msvcC++x64.exe