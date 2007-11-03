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
    int status, in = 0, out;

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

    status = I_SUCCESS;
    while (status != I_EXIT && status != I_ERROR)
    {
        if (status & I_INPUT)
            in = fgetc(stdin);
        status = interpreter_step(i, in, &out);
        if (status & I_OUTPUT)
        {
            fputc(out, stdout);
            fflush(stdout);
        }
    }
    return status != I_EXIT;
}
