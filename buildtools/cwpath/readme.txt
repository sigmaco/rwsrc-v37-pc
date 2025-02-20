CWPATH

Small command line app to retrieve the install path of CodeWarrior from the registry.

Usage

cwpath.exe %release_Ver"


e.g

cwpath.exe "PS2 R3.0"

would find the install location for CodeWarrior 3.0 for PS2, this actually searches the following reg key

HKEY_LOCAL_MACHINE\\Software\\Metrowerks\\CodeWarrior\\Product Versions\\PS2 R3.0\\PATH


cwpath.exe "GAMECUBE R2.0"

would find the install location for CodeWarrior R2.0 for GCN, this actually searches the following reg key

HKEY_LOCAL_MACHINE\\Software\\Metrowerks\\CodeWarrior\\Product Versions\\GAMECUBE R2.0\\PATH
