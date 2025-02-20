CC=cl

CFLAGS=\
	-c -DWIN32 -D_DEBUG  \
	-DVC_EXTRALEAN -DWIN32_EXTRA_LEAN -DWIN32_LEAN_AND_MEAN \
	-GB -GZ -MLd -Ob0 -Od -Oy- -UNDEBUG -W3 -Zi -nologo

LFLAGS =\
	-debug -incremental:no -machine:I386 -nologo -pdb:none \
	-subsystem:console -warn:3


all: incgen.exe 

incgen.exe: lexer.o main.o
	link $(LFLAGS) /out:$@ $^

%.c: %.l
	flex -o$@ $<

%.o: %.c
	$(CC) $(CFLAGS) -Fo$@ $<

clean:
	rm *.o
