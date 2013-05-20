
#include <stdio.h>

int main(int argc, char *argv[])
{

    int i;
    printf("test: amnt of params %d\n",argc);
    for(i=0;i<argc;i++)
        printf("test: Param %d '%s'\n",i,argv[i]);
    fflush(stdout);
    return 0;
}
