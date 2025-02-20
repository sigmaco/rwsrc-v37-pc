#ifndef MAIN_H
#define MAIN_H

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

void AddChild(char *name);
void AddFile(char *name);
char *ExpandPath(char *name);

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* MAIN_H */