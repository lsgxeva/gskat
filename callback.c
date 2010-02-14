#include <gtk/gtk.h>
#include "def.h"
#include "utils.h"
#include "game.h"
#include "interface.h"

gboolean quit(GtkWidget *window, gpointer data)
{
    gtk_main_quit();

    return TRUE;
}

gboolean realization(GtkWidget *area, gpointer data)
{
    struct _app *app = (struct _app *) data;

    /* allocate memory for app lists */
    alloc_app(app);

    if (load_cards(DATA_DIR, app, area))
        game_start(app);

    return FALSE;
}

void next_round(GtkButton *button, gpointer data)
{
    struct _app *app = (struct _app *) data;

    if (app->state == ENDGAME)
    {
        game_start(app);

        app->state = WAITING;

        next_round(button, data);
    }
    else if (app->state == WAITING)
    {
        app->state = PROVOKE;

        start_provoke(app);

        app->state = TAKESKAT;

        /* check if game was aborted
         * i.e. all players have passed */
        if (app->re)
        {
            take_skat(app);

            if (!app->re->human)
            {
                spiel_ansagen(app);

                app->state = PLAYING;
            }
        }
    }
    else if (app->state == TAKESKAT)
    {
        spiel_ansagen(app);

        app->state = PLAYING;
    }
}

gboolean configure(GtkWidget *area, GdkEventExpose *event, gpointer data)
{
    struct _app *app = (struct _app *) data;

    alloc_target(app);

    calc_card_positions(app);

    return TRUE;
}

gboolean button_press(GtkWidget *area, GdkEventButton *event, gpointer data)
{
    gboolean found = FALSE;
    struct _app *app = (struct _app *) data;

    if (event->button == 1)
    {
        if (app->state == TAKESKAT && app->re == app->players[0])
            found = click_skat(app, event);
        else if (app->state == PLAYING)
            found = play_card(app, event);
    }

    return found;
}

void refresh(GtkWidget *area, GdkEventExpose *event, gpointer data)
{
    struct _app *app = (struct _app *) data;

    if (app->target && app->area && app->area->window)
        draw_area(app);
}

/* vim:set et sw=4 ts=4 tw=80: */
