#include "windows.h"
#include "stdio.h"

int
main(int argc, char **argv)
{
    char lookuppath[1024];
    char path[1024];
    int pathlength = 1024;
    HKEY openedkey;
    int result;

    if (argc < 2)
    {
        return -1;
    }

    sprintf(lookuppath, "Software\\Metrowerks\\CodeWarrior\\Product Versions\\%s", argv[1]);

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lookuppath, 0, KEY_READ, &openedkey);

    if (result == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(openedkey, "PATH", NULL, NULL, path, &pathlength) == ERROR_SUCCESS)
        {
            printf("%s", path);
        }

        RegCloseKey(openedkey);
    }

    return (0);
}