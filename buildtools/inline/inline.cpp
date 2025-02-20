#if (defined(_MSC_VER))
#pragma warning(disable: 4786)
#endif /* (defined(_MSC_VER)) */

#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#define LINEBUFFER (1024)
#define STARTBLOCK "/* RWPUBLIC */"
#define ENDBLOCK   "/* RWPUBLICEND */"
#define INCLUDEBLOCK "#include"

#define INPUTFILE "-I"
#define OUTPUTFILE "-O"

typedef std::vector<std::string> StringArray;

static void
parse(std::istream &iStream, std::ostream &oStream, StringArray &paths)
{
    bool output;
    char data[LINEBUFFER];

    std::string startBlock(STARTBLOCK);
    std::string endBlock(ENDBLOCK);
    std::string includeBlock(INCLUDEBLOCK);

    output = false;

    while(iStream.getline(data, LINEBUFFER, '\n'))
    {
        std::string line(data);

        if(0 == strncmp(startBlock.c_str(), line.c_str(), startBlock.length()))
        {
            output = true;
        }
        else if(0 == strncmp(endBlock.c_str(), line.c_str(), endBlock.length()))
        {
            output = false;
        }
        else if(output)
        {
            if(0 == strncmp(includeBlock.c_str(), line.c_str(), includeBlock.length()))
            {
                size_t path;
                std::string::size_type pos;

                /* Open and recurse. */
                std::string file(line);

                pos = file.find_first_of('"');

                if (pos != std::string::npos)
                {
                    file.erase(0, pos + 1);
                    file.erase(file.find_first_of('"'), file.size());
                }
                else
                {
                    file.erase(0, file.find_first_of('<') + 1);
                    file.erase(file.find_first_of('>'), file.size());
                }

                for( path = 0; path < paths.size(); path++ )
                {
                    std::string filename;
                    std::ifstream recurse;

                    filename = paths[path] + std::string("/") + file;

                    recurse.open(filename.c_str());

                    if(recurse.is_open())
                    {
                        oStream << "/*---- start: " << filename;
                        oStream << "----*/" << std::endl;
                        parse(recurse, oStream, paths);
                        oStream << "/*---- end: " << filename;
                        oStream << "----*/" << std::endl;

                        recurse.close();

                        break;
                    }
                }

                if(path == paths.size())
                {
                    oStream << line << std::endl;
                }
            }
            else
            {
                oStream << line << std::endl;
            }
        }
    }

    return;
}

int
main(int argc, char **argv)
{
    StringArray paths;

    std::string iFile(INPUTFILE);
    std::string oFile(OUTPUTFILE);

    std::ifstream iStream;
    std::ofstream oStream;

    for(int i = 1; i < argc; i++)
    {
        std::string arg(argv[i]);

        if(0 == strncmp(arg.c_str(), iFile.c_str(), iFile.length()))
        {
            iStream.open(arg.c_str() + iFile.length());
        }
        else if(0 == strncmp(arg.c_str(), oFile.c_str(), oFile.length()))
        {
            oStream.open(arg.c_str() + oFile.length());
        }
        else
        {
            paths.push_back(arg);
        }
    }

    parse(iStream, oStream, paths);

    oStream << std::endl;

    oStream.close();

    return 0;
}
