#include "interpreter.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define IO_NONE    256
#define IO_BLOCK   257

const int DR[4] = {  0, +1,  0, -1 };
const int DC[4] = { +1,  0, -1,  0 };

static __inline__ char get(struct Interpreter *i, int row, int col)
{
    assert(row >= 0 && row < i->fld_sz.height &&
           col >= 0 && col < i->fld_sz.width);
    return i->field[i->fld_cap.width*row + col];
}

static __inline__ void set(struct Interpreter *i, int row, int col, char value)
{
    assert(row >= 0 && row < i->fld_sz.height &&
           col >= 0 && col < i->fld_sz.width);
    i->field[i->fld_cap.width*row + col] = value;
}

static __inline__ void add(struct Interpreter *i, int row, int col, int value)
{
    assert(row >= 0 && row < i->fld_sz.height &&
           col >= 0 && col < i->fld_sz.width);
    i->field[i->fld_cap.width*row + col] += value;
}

static __inline__ int cursor_needs_input(struct Interpreter *i, struct Cursor *c)
{
    if (c->dm != M_INPUT)
        return 0;
    switch (get(i, c->ir, c->ic))
    {
    case '^':
        return c->dr != 0;
    case '>':
    case '<':
    case 'v':
    case 'X':
        return 1;
    default:
        return 0;
    }
}

/* Ensures the field size is at least height x width, and reallocates
   the field buffer if necessary. */
static void ensure(struct Interpreter *i, int height, int width)
{
    if (width > i->fld_cap.width || height > i->fld_cap.height)
    {
        struct Size cap = i->fld_cap;
        char *f;
        int r, c;

        if (cap.width == 0)
            cap.width = 16;
        while (cap.width < width)
            cap.width *= 2;
        if (cap.height == 0)
            cap.height = 16;
        while (cap.height < height)
            cap.height *= 2;
        f = malloc(cap.width * cap.height);
        assert(f);
        memset(f, 0, cap.width * cap.height);
        for (r = 0; r < i->fld_cap.height; ++r)
            for (c = 0; c < i->fld_cap.width; ++c)
                f[cap.width*r + c] = i->field[i->fld_cap.width*r + c];
        free(i->field);
        i->field = f;
        i->fld_cap = cap;
    }

    if (width > i->fld_sz.width)
        i->fld_sz.width = width;
    if (height > i->fld_sz.height)
        i->fld_sz.height = height;
}

static void set_effect(struct Interpreter *i, struct Cursor *c)
{
    switch(c->dm)
    {
    case M_NONE:
        break;
    case M_ADD:
        c->effect = E_ADD | ((+get(i, c->dr, c->dc))&0xff);
        break;
    case M_SUBTRACT:
        c->effect = E_ADD | ((-get(i, c->dr, c->dc))&0xff);
        break;
    case M_INPUT:
        c->effect = E_INPUT;
        break;
    case M_OUTPUT:
        {
            int ch = get(i, c->dr, c->dc);
            if (i->output == IO_NONE)
                i->output = ch;
            else
            if (i->output != ch)
                i->output = IO_BLOCK;
            break;
        }
    case M_CLEAR:
        c->effect = E_CLEAR;
        break;
    }
}

static void move_ip(struct Interpreter *i, struct Cursor *c)
{
    c->ir += DR[c->id];
    c->ic += DC[c->id];
    while (c->ic < 0)
        c->ic += i->fld_sz.width;
    while (c->ic >= i->fld_sz.width)
        c->ic -= i->fld_sz.width;
}

static struct Cursor **kill_cursor(struct Cursor **ptr)
{
    struct Cursor *c = *ptr;
    *ptr = c->next;
    free(c);
    return ptr;
}

static struct Cursor **fork_cursor(struct Interpreter *i, struct Cursor **ptr)
{
    struct Cursor *b = malloc(sizeof(struct Cursor)), *c = *ptr;
    assert(b);
    *b = *c;
    b->next = c;
    *ptr = b;
    b->id ^= 1;
    c->id ^= 3;
    move_ip(i, b);
    return &b->next;
}

/* Executes a single instruction and returns a pointer to
   a pointer to the next cursor. */
static struct Cursor **run_cursor(struct Interpreter *i, struct Cursor **ptr)
{
    struct Cursor *c = *ptr;

    /* Reset field effect */
    c->effect = 0;

    /* Evaluate instruction */
    switch (get(i, c->ir, c->ic))
    {
        /* Move data pointer */
    case '>':
        set_effect(i, c);
        if (++c->dc == i->fld_sz.width)
            c->dc = 0;
        break;
    case 'v':
        set_effect(i, c);
        if (++c->dr == i->fld_sz.height)
            ensure(i, c->dr + 1, 0);
        break;
    case '<':
        set_effect(i, c);
        if (c->dc-- == 0)
            c->dc = i->fld_sz.width - 1;
        break;
    case '^':
        if (c->dr == 0)
            return kill_cursor(ptr);
        set_effect(i, c);
        --c->dr;
        break;
    case 'X':
        set_effect(i, c);
        break;

        /* Change data mode */
    case '~': c->dm = M_NONE; break;
    case '+': c->dm = M_ADD; break;
    case '-': c->dm = M_SUBTRACT; break;
    case '?': c->dm = M_INPUT; break;
    case '!': c->dm = M_OUTPUT; break;
    case '*':
        if (i->flags & F_CLEAR_MODE)
            c->dm = M_CLEAR;
        break;

        /* Change instruction pointer direction */
    case '\\': c->id ^= 1; break;
    case '/' : c->id ^= 3; break;
    case '|' : c->id ^= 2; break;

        /* Jumps */
    case '#':
        move_ip(i, c);
        break;
    case '@':
        if (get(i, c->dr, c->dc) == 0)
        {
            c->ir += DR[c->id];
            c->ic += DC[c->id];
        }
        break;

        /* Fork */
    case 'Y':
        ptr = fork_cursor(i, ptr);
        break;
    }

    /* Move instruction pointer */
    move_ip(i, c);

    return &c->next;
}

int interpreter_step(struct Interpreter *i, int in, int *out)
{
    struct Cursor **ptr, *c;
    int result;

    if (!i->cursors)
        return I_EXIT;

    result = I_SUCCESS;
    i->output = IO_NONE;

    /* Run cursors */
    ptr = &i->cursors;
    while (ptr && *ptr)
        ptr = run_cursor(i, ptr);

    /* Apply input read */
    if ((in & ~255) == 0)
    {
        for (c = i->cursors; c; c = c->next)
            if ((c->effect&E_MASK) == E_INPUT)
                set(i, c->dr, c->dc, in);
    }

    /* Apply additions/subtractions */
    for (c = i->cursors; c; c = c->next)
        if ((c->effect&E_MASK) == E_ADD)
            add(i, c->dr, c->dc, c->effect);

    /* Clear cells */
    if (i->flags & F_CLEAR_MODE)
    {
        for (c = i->cursors; c; c = c->next)
            if ((c->effect&E_MASK) == E_CLEAR)
                set(i, c->dr, c->dc, 0);
    }

    /* Remove cursors with invalid IP */
    ptr = &i->cursors;
    while (ptr && *ptr)
    {
        c = *ptr;
        if (c->ir < 0 || c->ir >= i->fld_sz.height)
        {
            ptr = kill_cursor(ptr);
            continue;
        }
        if (cursor_needs_input(i, c))
            result |= I_INPUT;
        ptr = &(*ptr)->next;
    }

    /* Write output */
    if (i->output < 256)
    {
        *out = i->output;
        result |= I_OUTPUT;
    }

    return result;
}

struct Size interpreter_size(struct Interpreter *i)
{
    return i->fld_sz;
}

int interpreter_needs_input(struct Interpreter *i)
{
    struct Cursor *c;

    for  (c = i->cursors; c; c = c->next)
        if (cursor_needs_input(i, c))
            return 1;
    return 0;
}

struct Interpreter *interpreter_create()
{
    struct Interpreter *i;

    i = malloc(sizeof(struct Interpreter));
    if (!i)
        goto failed;
    memset(i, 0, sizeof(struct Interpreter));

    /* Allocate initial cursor */
    i->cursors = malloc(sizeof(struct Cursor));
    if (!i->cursors)
        goto failed;
    memset(i->cursors, 0, sizeof(struct Cursor));

    /* Set initial 1x1 field */
    ensure(i, 1, 1);
    return i;

failed:
    interpreter_destroy(i);
    return NULL;
}

struct Interpreter *interpreter_clone(struct Interpreter *i)
{
    struct Interpreter *j;
    struct Cursor *c, *d;

    j = malloc(sizeof(struct Interpreter));
    if (!j)
        goto failed;
    memset(j, 0, sizeof(struct Interpreter));

    /* Duplicate flags */
    j->flags = i->flags;

    /* Duplicate cursors */
    for (c = i->cursors; c; c = c->next)
    {
        d = malloc(sizeof(struct Cursor));
        if (!d)
            goto failed;
        memcpy(d, c, sizeof(struct Cursor));
        d->next = j->cursors;
        j->cursors = d;
    }

    /* Duplicate field */
    j->fld_sz  = i->fld_sz;
    j->fld_cap = i->fld_cap;
    j->field = malloc(j->fld_cap.height * j->fld_cap.width);
    if (!j->field)
        goto failed;
    memcpy(j->field, i->field, j->fld_cap.height * j->fld_cap.width);

    return j;

failed:
    interpreter_destroy(j);
    return NULL;
}

void interpreter_destroy(struct Interpreter *i)
{
    struct Cursor *c, *next;

    c = i->cursors;
    while (c)
    {
        next = c->next;
        free(c);
        c = next;
    }
    i->cursors = NULL;
    free(i->field);
    i->field = NULL;
    free(i);
}

struct Interpreter *interpreter_from_source(const char *filepath, char nul)
{
    struct Interpreter *i;
    FILE *fp;
    char buf[512];
    size_t read;
    int row, col, n;

    fp = fopen(filepath, "rt");
    if (!fp)
        return NULL;

    i = interpreter_create();
    if (!i)
    {
        fclose(fp);
        return NULL;
    }
    row = col = 0;
    while ((read = fread(buf, 1, sizeof(buf), fp)) > 0)
    {
        for (n = 0; n < (int)read; ++n)
        {
            if (buf[n] == '\n')
            {
                col = 0;
                row += 1;
                continue;
            }
            ensure(i, row + 1, col + 1);
            set(i, row, col, buf[n] == nul ? 0 : buf[n]);
            col += 1;
        }
    }
    fclose(fp);
    return i;
}

char interpreter_get(struct Interpreter *i, int row, int col)
{
    return get(i, row, col);
}

void interpreter_set(struct Interpreter *i, int row, int col, char value)
{
    return set(i, row, col, value);
}

void interpreter_add(struct Interpreter *i, int row, int col, int value)
{
    return add(i, row, col, value);
}


int interpreter_get_flags(struct Interpreter *i)
{
    return i->flags;
}

int interpreter_set_flags(struct Interpreter *i, int flags)
{
    return i->flags = (flags & F_ALL);
}

int interpreter_add_flags(struct Interpreter *i, int flags)
{
    return i->flags |= (flags & F_ALL);
}
