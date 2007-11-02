#include "interpreter.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    char nul = 0, ch;
    struct Interpreter *i;

    while ((ch = getopt(argc, argv, "c:")) != -1)
    {
        switch (ch)
        {
        case 'c':
            if (strlen(optarg) != 1)
            {
                printf("-c expects a single character, not \"%s\"\n", optarg);
                return 1;
            }
            nul = *optarg;
            break;
        }
    }
    if (argc - optind != 1)
    {
        printf("Usage: %s [-cx] <program>\n", argv[0]);
        return argc != 1;
    }

    i = interpreter_from_source(argv[optind], nul);
    if (!i)
    {
        fprintf(stderr, "Could not open %s\n", argv[optind]);
        return 1;
    }
    while (interpreter_step(i) == 0) { };
    interpreter_destroy(i);
    return 0;
}
