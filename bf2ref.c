#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Mapping of Brainfuck instructions to Refunge instruction sequences:

    <       >       +        -      .       ,       [       ]
   ---     ---     ---      ---    ---     ---     ---     ---
    <       <       <        <      !       ?       @       @
    +       +       +        -      X       X       #    \../../ 
    ^       v       >        >      ~       ~    /..\..\
    -       -       ~        ~
    v       ^
    ~       ~
    >       >
    ^       v
*/

void space(int count)
{
    while (count-- > 0)
        putchar(' ');
}

void write(char *p, int indent)
{
    while (*p)
    {
        space(indent);
        putchar(*p++);
        putchar('\n');
    }
}

int main()
{
    char *prog_buf;
    int prog_cap, prog_len;
    int level, max_level, n;

    /* Read program */
    prog_buf = NULL;
    prog_len = prog_cap = 0;
    do {
        switch (n = getchar())
        {
        case '<': case '>':
        case '+': case '-':
        case '.': case ',':
        case '[': case ']':
            if (prog_len == prog_cap)
            {
                prog_cap = prog_cap ? 2*prog_cap : 128;
                prog_buf = realloc(prog_buf, prog_cap);
                assert(prog_buf);
            }
            prog_buf[prog_len++] = n;
        }
    } while (n != EOF);

    /* Determine maximum nesting level */
    level = max_level = 0;
    for (n = 0; n < prog_len; ++n)
    {
        if (prog_buf[n] == '[')
        {
            ++level;
            if (level > max_level)
                max_level = level;
        }
        else
        if (prog_buf[n] == ']')
        {
            --level;
            assert(level >= 0);
        }
    }
    assert(level == 0);

    /* Write prologue */
    puts("UT>-<*>~v<@#\\");
    puts("  /  > /#@\\#/");
    puts("       \\^ /");
    /* FIXME: right entry point */
    if (max_level > 0)
    {
        space(2);
        putchar('\\');
        space(max_level - 1);
        putchar('\\');
        putchar('\n');
    }

    /* Write program instructions */
    for (n = 0; n < prog_len; ++n)
    {
        switch (prog_buf[n])
        {
        case '<':
            write("<+^-v~>^", 2 + max_level);
            break;
        case '>':
            write("<+v-^~>v", 2 + max_level);
            break;
        case '+':
            write("<+>~", 2 + max_level);
            break;
        case '-':
            write("<->~", 2 + max_level);
            break;
        case '.':
            write("!X~", 2 + max_level);
            break;
        case ',':
            write("?X~", 2 + max_level);
            break;
        case '[':
            ++level;
            write("@#", 2 + max_level);
            space(2 + max_level - level);
            putchar('/');
            space(level - 1);
            putchar('\\');
            space(level - 1);
            putchar('\\');
            putchar('\n');
            break;
        case ']':
            write("@", 2 + max_level);
            space(2 + max_level - level);
            putchar('\\');
            space(level - 1);
            putchar('/');
            space(level - 1);
            putchar('/');
            putchar('\n');
            --level;
            break;
        }
    }
    return 0;
}
