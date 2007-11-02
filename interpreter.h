#ifndef INTERPRETER_H_INCLUDED
#define INTERPRETER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

extern const int DR[4], DC[4];

struct Size {
    int width, height;
};

enum Mode {
    M_NONE = 0, M_ADD = 1, M_SUBTRACT = 2, M_CLEAR = 3
};

#define E_INPUT 0x0100
#define E_CLEAR 0x0200

struct Cursor {
    struct Cursor *next;
    int ir, ic, id;
    int dr, dc;
    enum Mode dm;
    int effect;
};

struct Interpreter
{
    char *field;
    struct Size fld_sz, fld_cap;
    struct Cursor *cursors;
    int input, output;
};

struct Interpreter *interpreter_from_source(const char *filepath, char nul);
struct Interpreter *interpreter_create();
struct Interpreter *interpreter_clone(struct Interpreter *i);
void interpreter_destroy(struct Interpreter *i);
char interpreter_get(struct Interpreter *i, int height, int width);
void interpreter_put(struct Interpreter *i, int height, int width, char value);
struct Size interpreter_size(struct Interpreter *i);
int interpreter_step(struct Interpreter *i);

#ifdef __cplusplus
}
#endif

#endif /* ndef INTERPRETER_H_INCLUDED */
