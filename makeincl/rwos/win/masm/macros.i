RWASM386				equ		.386
RWASMINCLUDE			equ		include

RWASMEXTERN macro name
	extern C name:DWORD
	    endm

RWASMFUNC   macro name
	public C name
name:
			endm

RWASMDATASEGMENTBEGIN	equ		_rwdseg segment dword public use32 'DATA'
RWASMDATASEGMENTEND		equ		_rwdseg	ends

RWASMCODESEGMENTBEGIN	macro
		_rwcseg	segment dword public use32 'CODE'
		assume cs:_rwcseg,ds:_rwdseg,es:_rwdseg
						endm
RWASMCODESEGMENTEND		equ		_rwcseg	ends

RWASMDEFINE				macro name,value
	name = value
						endm
