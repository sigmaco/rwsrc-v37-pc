# Change st(n) to stn where n is 0 to 7
s/[Ss][Tt](\([0-7]\))/st\1/g

# Change a include file to a %include "file"
s/\(^[ 	]*\)[iI][nN][cC][lL][uU][dD][eE]\([ 	]*\)\([^;]*\)/\1%include\2"\3"/

# Prepend % in front of if, ifdef, elif, else and endif
s/\(^[ 	]*\)[iI][fF]/\1%if/
s/\(^[ 	]*\)[iI][fF][dD][eE][fF]/\1%ifdef/
s/\(^[ 	]*\)[eE][lL][iI][fF]/\1%elif/
s/\(^[ 	]*\)[eE][lL][sS][eE]/\1%else/
s/\(^[ 	]*\)[eE][nN][dD][iI][fF]/\1%endif/

# Remove masm's label before the segment keyword
s/^[ 	]*[A-Za-z0-9_]\+[ 	]\+\([sS][eE][gG][mM][eE][nN][tT][ 	]\).*'DATA'/	\1.data/
s/^[ 	]*[A-Za-z0-9_]\+[ 	]\+\([sS][eE][gG][mM][eE][nN][tT][ 	]\).*'CODE'/	\1.text/
s/^[ 	]*[A-Za-z0-9_]\+[ 	]\+[eE][nN][dD][sS]//

# Change the st to st0  
s/\([, 	]st\)\([, 	]\)/\10\2/g

# Equivilent BITS 32 to masms .386
s/\(^[ 	]*\).386/\1BITS 32/
s/^[ 	]*assume[ 	]\+.*//
