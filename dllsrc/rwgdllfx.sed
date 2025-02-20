# SED file for fixing the .def file required for DLL generation

# trim off ' @ <n> ;' on the tail end of lines
s/\(^.*\) @ \([0-9]\+ ;\)/\1/

# trim off ' @ <n> DATA ;' on the tail end of lines
s/\(^.*\) @ \([0-9]\+ DATA ;\)/\1/

# we really don't want lines of the form '_real@<hex>'
/\_real@\([0-9a-fA-f]\)/d

# we don't want lines beginning with a quote " either 
/\"/d

# .NET builds require extra culling
/AVexception/d
/length_error/d
/logic_error/d
/out_of_range/d
