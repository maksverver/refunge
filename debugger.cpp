#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <algorithm>
#include <utility>
#include <vector>

#ifdef _MSC_VER     /* WIN32 */
#include <getopt.h>
#else               /* POSIX */
#include <unistd.h>
#include <fcntl.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Scroll.H>
#include <FL/fl_draw.H>

#define Cursor Refunge_Cursor
#include "interpreter.h"

const int SIZE = 20, FONT_SIZE = 15;
const int FAST_STEPS = 10000;

static Interpreter *initial, *i;
static int i_in, i_out, i_steps;
static Size size;
static class CellWidget **widgets;
static Fl_Window *window;
static Fl_Button *start_button, *fast_button, *step_button, *reset_button;
static Fl_Output *counter;
static bool fast = false;
static Fl_Scroll *cell_group;

static const int COLORS_SIZE = 12;
static const Fl_Color COLORS[COLORS_SIZE] = {
    fl_rgb_color(255,0,0), fl_rgb_color(0,240,0), fl_rgb_color(0,0,255),
    fl_rgb_color(240,240,0), fl_rgb_color(240,0,240), fl_rgb_color(0,240,240),
    fl_rgb_color(240,128,0), fl_rgb_color(240,0,128), fl_rgb_color(0,240,128),
    fl_rgb_color(128,240,0), fl_rgb_color(128,0,240), fl_rgb_color(0,128,240) };

class CellWidget : public Fl_Widget
{
    int r, c;
    Fl_Color border[2], arrow[4];
    bool brk;

public:
    CellWidget(int x, int y, int w, int h, int r, int c)
    : Fl_Widget(x, y, w, h, ""), r(r), c(c), brk(0)
    {
        clear();
    }

    int handle(int event)
    {
        switch (event)
        {
        case FL_PUSH:
            if ( Fl::event_button1() && !Fl::event_button2() && !Fl::event_button3() &&
                 Fl::event_shift() && !Fl::event_ctrl() && !Fl::event_alt() )
            {
                brk = !brk;
                damage(1);
                return 1;
            }
            break;
        }
        return 0;
    }

    void draw()
    {
        char buf[4];
        int ch = interpreter_get(i, r, c);

        fl_rectf(x(), y(), w(), h(), brk ? FL_YELLOW : FL_WHITE);
        if (border[0] == FL_BLACK)
            fl_rect(x(), y(), w(), h(), FL_BLACK);
        else
        {
            fl_rect(x() + 0, y() + 0, w() - 0, h() - 0, border[0]);
            fl_rect(x() + 1, y() + 1, w() - 2, h() - 2, border[0]);
            if (border[1] != FL_BLACK)
            {
                fl_rect(x() + 2, y() + 2, w() - 4, h() - 4, border[1]);
                fl_rect(x() + 3, y() + 3, w() - 6, h() - 6, border[1]);
            }
        }

        for (int d = 0; d < 4; ++d)
        {
            if (arrow[d] == FL_BLACK)
                continue;
            fl_color(arrow[d]);
            int ax = x() + 2,       ay = y() + 2,
                bx = x() + w() - 3, by = y() + 2,
                cx = x() + 2,       cy = y() + h() - 3,
                dx = x() + w() - 3, dy = y() + h() - 3;
            switch (d)
            {
            case 0:
                fl_polygon(ax,ay, cx,cy, (bx+dx)/2,(by+dy)/2);
                break;
            case 1:
                fl_polygon(ax,ay, bx,by, (cx+dx)/2,(cy+dy)/2);
                break;
            case 2:
                fl_polygon(bx,by, dx,dy, (ax+cx)/2,(ay+cy)/2);
                break;
            case 3:
                fl_polygon(cx,cy, dx,dy, (ax+bx)/2,(ay+by)/2);
                break;
            }
        }

        if (ch >= 32 && ch < 127)
        {
                sprintf(buf, "%c", ch);
                fl_color(FL_BLACK);
                fl_font(1, FONT_SIZE);
                fl_draw(buf, x() + 4, y() + SIZE - 4);
        }
        else
        {
            sprintf(buf, "%02X", ((unsigned)ch)&0xff);
            fl_color(FL_BLUE);
            fl_font(0, FONT_SIZE - 3);
            fl_draw(buf, x() + 2, y() + SIZE - 4);
        }
    }

    void clear()
    {
        border[0] = border[1] = FL_BLACK;
        arrow[0] = arrow[1] = arrow[2] = arrow[3] = FL_BLACK;
        damage(1);
    }

    void addDataPointer(Fl_Color c)
    {
        if (border[0] == c || border[1] == c)
            return;
        if (border[0] == FL_BLACK)
            border[0] = c;
        else
            border[1] = c;
        damage(1);
    }

    void addInstructionPointer(Fl_Color c, int d)
    {
        arrow[d] = c;
        damage(1);
    }

    bool breakpoint()
    {
        return brk;
    }
};

void decorate_cells()
{
    static std::vector< std::pair<Cursor*, Fl_Color> > cursor_color;
    static int next_color = 0;

    std::vector< std::pair<Cursor*, Fl_Color> >::iterator pos = cursor_color.begin();
    for (Cursor *c = i->cursors; c; c = c->next)
    {
        std::vector< std::pair<Cursor*, Fl_Color> >::iterator it = pos;
        while (it != cursor_color.end() && it->first != c) ++it;
        if (it == cursor_color.end())
        {
            pos = cursor_color.insert(pos, std::make_pair(c, COLORS[next_color]));
            if (++next_color == COLORS_SIZE) next_color = 0;
        }
        else
        {
            if (it != pos) std::rotate(pos, it, cursor_color.end());
        }
        Fl_Color col = pos++->second;
        widgets[size.width * c->dr + c->dc]->addDataPointer(col);
        widgets[size.width * c->ir + c->ic]->addInstructionPointer(col, c->id);
    }
    cursor_color.erase(pos, cursor_color.end());
}

void create_widgets(Size sz)
{
    cell_group->begin();
    CellWidget **ws = new CellWidget*[sz.width*sz.height];
    for (int r = 0; r < sz.height; ++r)
        for (int c = 0; c < sz.width; ++c)
        {
            int n = r*sz.width + c;
            ws[n] = (r < size.height && c < size.width)
                ? widgets[r*size.width + c]
                : new CellWidget(SIZE*c + cell_group->x() - cell_group->xposition(),
                                 SIZE*r + cell_group->y() - cell_group->yposition(),
                                 SIZE, SIZE, r, c);
        }
    cell_group->end();
    delete[] widgets;
    widgets = ws;
    size = sz;
}


void simulate_step(void *arg)
{
    for (Cursor *c = i->cursors; c; c = c->next)
    {
        widgets[size.width * c->dr + c->dc]->clear();
        widgets[size.width * c->ir + c->ic]->clear();
    }

    bool brk = false;
    for (int n = 0; n < (fast ? FAST_STEPS : 1) && !brk; ++n)
    {
        i_in = -1;
        if (interpreter_needs_input(i))
        {
            char ch;
            ssize_t res = read(0, &ch, 1);
#ifndef _MSC_VER
            if (res < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
#endif
            if (res > 0) i_in = ch & 0xff;
        }
        if (interpreter_step(i, i_in, &i_out) & I_OUTPUT)
        {
            fputc(i_out, stdout);
            fflush(stdout);
        }
        ++i_steps;
        for (Cursor *c = i->cursors; c; c = c->next)
            if ( c->ir < size.height && c->id < size.width &&
                 widgets[size.width*c->ir + c->ic]->breakpoint() )
                brk = true;
    }
 
    char buf[16];
    sprintf(buf, "%d", i_steps);
    counter->value(buf);

    Size sz = interpreter_size(i);
    if (sz.height > size.height)
        create_widgets(sz);
    decorate_cells();
    cell_group->redraw();

    if (arg == 0 && !brk)
        Fl::repeat_timeout(0.1, simulate_step);
}

void button_callback(Fl_Widget *widget, void *arg)
{
    Fl::remove_timeout(simulate_step);

    fast = widget == fast_button;
    if (widget == step_button)
        simulate_step(widget);
    if (widget == start_button || widget == fast_button)
        Fl::add_timeout(0.1, simulate_step);
}

void reset_callback(Fl_Widget *widget, void *arg)
{
    Fl::remove_timeout(simulate_step);
    interpreter_destroy(i);
    i = interpreter_clone(initial);
    i_steps = 0;
    size.width = size.height = 0;
    counter->value("0");
    cell_group->clear();
    create_widgets(interpreter_size(i));
    cell_group->redraw();
}

void run_debugger()
{
    window = new Fl_Double_Window(400, 300, "Refunge Debugger");
    start_button = new Fl_Button(0, 0, 50, 25, "Slow");
    start_button->callback(button_callback);
    fast_button = new Fl_Button(50, 0, 50, 25, "Fast");
    fast_button->callback(button_callback);
    step_button = new Fl_Button(100, 0, 50, 25, "Step");
    step_button->callback(button_callback);
    reset_button = new Fl_Button(150, 0, 50, 25, "Reset");
    reset_button->callback(reset_callback);
    counter = new Fl_Output(200, 0, 100, 25);
    counter->value("0");
    window->end();

    cell_group = new Fl_Scroll(0, 25, 400, 250);
    cell_group->end();
    create_widgets(interpreter_size(i));
    decorate_cells();

    window->add_resizable(*cell_group);
    window->size_range(200, 200);
    window->show();
    Fl::run();
}

int main(int argc, char *argv[])
{
    char nul = 0, ch;
    bool clear_mode = false;

    while ((ch = getopt(argc, argv, "*c:")) != -1)
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
        case '*':
            clear_mode = true;
            break;
        }
    }
    if (argc - optind != 1)
    {
        printf("Usage: %s [-*] [-cx] <program>\n", argv[0]);
        return argc != 1;
    }

#ifndef _MSC_VER
    /* Try to set stdin to non-blocking so the GUI doesn't hang on input. */
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
#endif

    initial = interpreter_from_source(argv[optind], nul);
    if (!initial)
    {
        printf("Could not create interpreter!\n");
        return 1;
    }
    if (clear_mode)
        interpreter_add_flags(initial, F_CLEAR_MODE);
    i = interpreter_clone(initial);
    run_debugger();
    interpreter_destroy(i);
    interpreter_destroy(initial);
    return 0;
}
