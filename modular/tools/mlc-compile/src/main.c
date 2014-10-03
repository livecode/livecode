#include <stdio.h>

extern void ROOT(void);

int main(int argc, char *argv[])
{
    if (argc != 2)
        return 1;
    
    extern int yydebug;
    yydebug = 1;
    
    extern FILE *yyin;
    yyin = fopen(argv[1], "r");
    if (yyin == NULL)
        return 1;
    
    ROOT();
    
    return 0;
}
