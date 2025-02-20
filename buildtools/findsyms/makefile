all: findsyms.exe 

findsyms.exe: findsyms.o
	link -nologo -warn:3 -out:$@ -debug $^

%.o: %.c
	cl -D_DEBUG -MLd -Od -UNDEBUG -W3 -Z7 -c -nologo -Fo$@ $<

clean:
	rm *.o
