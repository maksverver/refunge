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
    M_NONE = 0, M_ADD, M_SUBTRACT, M_INPUT, M_OUTPUT, M_CLEAR
};

/* Data effects for cursors */
#define E_MASK          0x0f00
#define E_ADD           0x0000
#define E_INPUT         0x0100
#define E_CLEAR         0x0200    /* Deprecated */

/* interpreter_step return values */
#define I_SUCCESS       0
#define I_ERROR         1    /* An error occured */
#define I_EXIT          2    /* Code terminated */
#define I_INPUT         4    /* Input required next step */
#define I_OUTPUT        8    /* Output generated last step */

/* Interpreter flags */
#define F_NONE          0
#define F_CLEAR_MODE    1
#define F_ALL           1

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
    int flags;
    int output; /* temp */
};

struct Interpreter *interpreter_from_source(const char *filepath, char nul);
struct Interpreter *interpreter_create();
struct Interpreter *interpreter_clone(struct Interpreter *i);
void interpreter_destroy(struct Interpreter *i);
char interpreter_get(struct Interpreter *i, int height, int width);
void interpreter_put(struct Interpreter *i, int height, int width, char value);
struct Size interpreter_size(struct Interpreter *i);
int interpreter_needs_input(struct Interpreter *i);
int interpreter_step(struct Interpreter *i, int in, int *out);
int interpreter_get_flags(struct Interpreter *i);
int interpreter_set_flags(struct Interpreter *i, int flags);
int interpreter_add_flags(struct Interpreter *i, int flags);

#ifdef __cplusplus
}
#endif

#endif /* ndef INTERPRETER_H_INCLUDED */
