%idefine	ptr

%macro		RWASM386 	0
		BITS	32
%endmacro
		
%ifdef ELF
	%macro		RWASMEXTERN	1
			extern %1
	%endmacro
	%macro		RWASMFUNC	1
			global %1	
	%1:
	%endmacro
%else
	%macro		RWASMEXTERN	1
			extern _%1
			%define %1 _%1
	%endmacro
	%macro		RWASMFUNC	1
			global _%1	
	_%1:
	%endmacro
%endif

%macro  	RWASMCODESEGMENTBEGIN	0
		SECTION .text
%endmacro
%define		RWASMCODESEGMENTEND


%macro  	RWASMDATASEGMENTBEGIN	0
		SECTION .data
%endmacro
%define		RWASMDATASEGMENTEND

;RWASMCODESEGMENTBEGIN	macro
;		_rwcseg	segment dword public use32 'CODE'
;		assume cs:_rwcseg,ds:_rwdseg,es:_rwdseg
;						endm
;RWASMCODESEGMENTEND		equ		_rwcseg	ends

%macro		RWASMDEFINE		1
		%define			%1
%endmacro
