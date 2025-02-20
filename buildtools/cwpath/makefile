CC=cl

CFLAGS=\
	-c -DWIN32 \
	-DVC_EXTRALEAN -DWIN32_EXTRA_LEAN -DWIN32_LEAN_AND_MEAN \
	-GB -MD -GX -Ob0 -Oy- -nologo

LFLAGS =\
	-incremental:no -machine:I386 -nologo -pdb:none \
	-subsystem:console -warn:1


all: cwpath.exe 

cwpath.exe: main.o
	link $(LFLAGS) /out:$@ $^ advapi32.lib

%.o: %.c
	$(CC) $(CFLAGS) -Fo$@ $<

clean:
	rm *.o

