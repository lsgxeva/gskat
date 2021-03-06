/*
 *  This file is part of gskat.
 *
 *  Copyright (C) 2010-2011 by Gregor Uhlenheuer
 *
 *  gskat is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  gskat is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gskat.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "def.h"
#include "callback.h"
#include "common.h"
#include "configuration.h"
#include "draw.h"
#include "game.h"
#include "gamestate.h"
#include "interface.h"
#include "license.h"

/**
 * quit:
 * @window:  #GtkWindow triggering the event
 * @data:    arbitrary user data
 *
 * Leave the gtk main loop and exit the program
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean quit(GtkWidget *window, gpointer data)
{
    UNUSED(window);
    UNUSED(data);

    gtk_main_quit();

    return TRUE;
}

/**
 * realization:
 * @area:  #GtkDrawingArea triggering the event
 * @data:  arbitrary user data
 *
 * Allocate memory for game objects, load all cards
 * and start the first game round
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean realization(GtkWidget *area, gpointer data)
{
    UNUSED(area);
    UNUSED(data);
    gint i;
    gboolean cards_loaded = FALSE;
    gboolean use_custom = gskat.datadir != NULL;

    const gchar **dirs = NULL;
    const gchar *default_dirs[] = { "img", "cards", "data", DATA_DIR, NULL };
    const gchar *custom_dir[] = { gskat.datadir };

    /* allocate memory for application lists */
    alloc_app();

    /* determine which directories to search in */
    dirs = use_custom ? custom_dir : default_dirs;

    /* load suit icons */
    load_suit_icons(dirs);

    /* load card images */
    cards_loaded = load_cards(dirs);

    if (cards_loaded)
        game_start();
    else
    {
        g_printerr("Failed to load all card images. "
                "Images were searched inside:\n");

        for (i=0; dirs[i]; i++)
            g_printerr("- %s\n", dirs[i]);

        if (!use_custom)
            g_printerr("You may want to use the '--data' command line argument "
                "to specify the directory where the card images are located.\n");

        g_printerr("Quit gskat.\n");

        exit(EXIT_FAILURE);
    }

    return FALSE;
}

gboolean window_close(GtkButton *button, gpointer data)
{
    UNUSED(button);

    gtk_widget_destroy((GtkWidget *) data);

    return TRUE;
}

/**
 * show_about_window:
 * @menuitem:  #GtkMenuItem emitting the signal
 * @data:      arbitrary user data
 *
 * Show the about dialog window
 */
void show_about_window(GtkMenuItem *menuitem, gpointer data)
{
    UNUSED(menuitem);
    UNUSED(data);

    gchar *img_file = g_build_filename(DATA_DIR, "icons",
            "gskat128x128.png", NULL);
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(img_file, NULL);

    const gchar *authors[] =
    {
        "Gregor Uhlenheuer <kongo2002@googlemail.com>",
        NULL
    };

    gtk_show_about_dialog(GTK_WINDOW(gskat.window),
            "program_name", _("gskat"),
            "comments", _("Gtk Skat game written in C"),
            "authors", authors,
            "artists", authors,
            "version", VERSION,
            "license", license_string,
            "logo", pixbuf,
            "website", "http://kongo2002.github.com",
            "copyright", "Copyright © 2010-2011 Gregor Uhlenheuer.\n"
                "All Rights Reserved.",
            NULL);

    if (pixbuf)
        g_object_unref(pixbuf);
    g_free(img_file);
}

/**
 * destroy_config:
 * @widget:  #GtkWidget receiving the signal
 * @event:   #GdkEvent triggering the signal
 * @data:    config dialog window widget
 *
 * Wrapper function to 'close_config' because of the different
 * function arguments of the 'delete-event' signal
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean destroy_config(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    UNUSED(widget);
    UNUSED(event);

    return close_config(NULL, data);
}

/**
 * close_config:
 * @button:  #GtkButton that was clicked
 * @data:    config dialog window widget
 *
 * Callback function of the 'cancel' button of the config dialog.
 *
 * Frees the allocated memory for the confwidgets array
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean close_config(GtkButton *button, gpointer data)
{
    UNUSED(button);

    GtkWidget **widgets = (GtkWidget **) data;

    gtk_widget_destroy(widgets[3]);
    g_free(widgets);

    return TRUE;
}

/**
 * save_bugreport:
 * @button:  #GtkButton that was clicked
 * @data:    #br_group object
 *
 * Callback function of the 'ok' button of the bugreport dialog.
 *
 * Saves the bug report into the user-defined directory.
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean save_bugreport(GtkButton *button, gpointer data)
{
    UNUSED(button);

    gchar *dir, *filename, *file;
    gchar date_string[256];
    GTimeVal time;
    GDate *date;

    br_group *bug_report_group = (br_group *) data;

    GtkWidget *window = bug_report_group->window;
    GtkTextBuffer *text_buffer = bug_report_group->text_buffer;
    GtkFileChooserButton *dir_chooser = GTK_FILE_CHOOSER_BUTTON(
            bug_report_group->file_chooser);

    /* get current time */
    g_get_current_time(&time);

    date = g_date_new();
    g_date_set_time_val(date, &time);
    g_date_strftime(date_string, 256, "%F", date);

    /* construct filename with random number to avoid duplicates */
    file = g_strdup_printf("gskat_bugreport_%s_%03d", date_string,
            g_random_int_range(1, 1000));

    /* determine bug report directory */
    dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dir_chooser));

    filename = g_build_filename(dir, file, NULL);

    /* write bug report to file */
    save_bugreport_to_file(filename, &time, text_buffer);

    gtk_widget_destroy(window);

    g_free(date);
    g_free(dir);
    g_free(file);
    g_free(filename);
    g_free(bug_report_group);

    return TRUE;
}

/**
 * destroy_bugreport:
 * @widget:  #GtkWidget receiving the signal
 * @event:   #GdkEvent triggering the signal
 * @data:    #br_group object
 *
 * Wrapper function to 'close_bugreport' because of the different
 * function arguments of the 'delete-event' signal
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean destroy_bugreport(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    UNUSED(widget);
    UNUSED(event);

    return close_bugreport(NULL, data);
}

/**
 * close_bugreport:
 * @button:  #GtkButton that was clicked
 * @data:    #br_group object
 *
 * Callback function of the 'cancel' button of the bugreport dialog.
 *
 * Frees the allocated widgets of the bug report window.
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean close_bugreport(GtkButton *button, gpointer data)
{
    UNUSED(button);

    br_group *bug_report_group = (br_group *) data;

    gtk_widget_destroy(bug_report_group->window);
    g_free(bug_report_group);

    return TRUE;
}

/**
 * destroy_show_trick:
 * @widget:  #GtkWidget receiving the signal
 * @event:   #GdkEvent triggering the signal
 * @data:    stich_view structure
 *
 * Wrapper function to 'close_show_trick' because of the different
 * function arguments of the 'delete-event' signal
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean destroy_show_trick(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    UNUSED(widget);
    UNUSED(event);

    return close_show_trick(NULL, data);
}

/**
 * close_show_trick:
 * @button:  #GtkButton that was clicked
 * @data:    stich_view structure
 *
 * Callback function of the 'show last trick' dialog window
 *
 * Frees all allocated memory belonging to the dialog window
 * including the stich_view structure
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean close_show_trick(GtkButton *button, gpointer data)
{
    UNUSED(button);

    stich_view *sv = (stich_view *) data;

    gtk_widget_destroy(sv->window);
    g_free(sv);

    return TRUE;
}

/**
 * prev_stich_click:
 * @button:  #GtkButton that was clicked
 * @data:    stich_view structure
 *
 * Callback function of the 'previous' button
 * of the 'show last trick' dialog window
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean prev_stich_click(GtkButton *button, gpointer data)
{
    stich_view *sv = (stich_view *) data;

    /* refresh the stich pointer */
    sv->stich = gskat.stiche[--sv->cur];

    /* deactivate button if on the first played stich of the round
     * or if the maximum number of viewable tricks is reached
     * according to the 'num_show_tricks' config value */
    if (sv->cur <= 0 || (gskat.stich - sv->cur) >= get_prop_int("num_show_tricks"))
        gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);

    /* activate next stich button */
    gtk_widget_set_sensitive(sv->nextb, TRUE);

    /* trigger stich drawing */
    draw_tricks_area(sv->area, sv);

    return TRUE;
}

/**
 * next_stich_click:
 * @button:  #GtkButton that was clicked
 * @data:    stich_view structure
 *
 * Callback function of the 'previous' button
 * of the 'show last trick' dialog window
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean next_stich_click(GtkButton *button, gpointer data)
{
    stich_view *sv = (stich_view *) data;

    /* refresh the stich pointer */
    sv->stich = gskat.stiche[++sv->cur];

    /* deactivate button if on the last played stich of the round */
    if (sv->cur >= (gskat.stich - 2))
        gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);

    /* activate previous stich button */
    gtk_widget_set_sensitive(sv->prevb, TRUE);

    /* trigger stich drawing */
    draw_tricks_area(sv->area, sv);

    return TRUE;
}

/**
 * save_config:
 * @button:  #GtkButton that was clicked
 * @data:    config dialog widgets
 *
 * Callback function of the 'apply' button of the config dialog.
 *
 * Apply the set values and free the allocated memory of the confwidgets
 * array afterwards
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean save_config(GtkButton *button, gpointer data)
{
    UNUSED(button);

    gint i;
    const gchar *cptr = NULL;
    gchar *filename = g_build_filename(get_config_dir(), "gskat.conf", NULL);
    GtkWidget **widgets = (GtkWidget **) data;

    /* change player names if differing */
    for (i=0; i<3; ++i)
    {
        cptr = gtk_entry_get_text(GTK_ENTRY(widgets[i]));

        if (strcmp(gskat.player_names[i], cptr))
        {
            g_free(gskat.player_names[i]);
            gskat.player_names[i] = g_strdup(cptr);

            /* refresh game area when a player name has changed */
            draw_area();
        }
    }

    g_hash_table_foreach(gskat.config, get_prop_widget_val, NULL);

    write_config(filename);

    gtk_widget_destroy(widgets[3]);
    g_free(filename);
    g_free(widgets);

    return TRUE;
}

/**
 * next_round:
 * @button:  #GtkButton that was clicked
 * @data:    arbitrary user data
 *
 * Start the next or first round of the game triggered
 * by a click on the 'New Round' button
 */
void next_round(GtkButton *button, gpointer data)
{
    if (gskat.state == ENDGAME)
    {
        game_start();

        gskat.state = WAITING;

        next_round(button, data);
    }
    else if (gskat.state == WAITING)
    {
        gskat.state = PROVOKE1;

        start_bidding();
    }
    else if (gskat.state == TAKESKAT)
    {
        spiel_ansagen();

        gskat.state = PLAYING;
    }
    /* abort the current round via the menu bar "Game -> New Round" */
    else
    {
        if (game_abort())
        {
            reset_game();

            gskat.state = ENDGAME;

            next_round(button, data);
        }
    }
}

/**
 * configure:
 * @area:   #GtkDrawingArea triggering the event
 * @event:  #GdkEventExpose structure
 * @data:   arbitrary user data
 *
 * Recalculate the card positions on the game table
 * after the window being resized
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean configure(GtkWidget *area, GdkEventExpose *event, gpointer data)
{
    UNUSED(area);
    UNUSED(event);
    UNUSED(data);

    calc_card_positions();

    return TRUE;
}

/**
 * mouse_move:
 * @area:   #GtkDrawingArea triggering the event
 * @event:  #GdkEventMotion structure
 * @data:   arbitrary user data
 *
 * Callback function of a mouse move event in the drawing area
 *
 * If the cursor is moving over a non-valid card the mouse cursor shape
 * is changed to a cross (or something the like).
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean mouse_move(GtkWidget *area, GdkEventMotion *event, gpointer data)
{
    UNUSED(data);

    gint num_cards = (gskat.table) ? g_list_length(gskat.table) : 0;
    GList *poss = NULL, *ptr;
    GdkWindow *window = area->window;
    GdkCursor *cursor = gdk_window_get_cursor(window);
    card *card;

    /* return if not enabled in configuration */
    if (!get_prop_bool("show_poss_cards"))
        return FALSE;

    /* check if it's the player's turn */
    if (gskat.state == PLAYING && ((gskat.cplayer + num_cards) % 3 == 0))
    {
        /* get possible cards */
        poss = get_possible_cards(gskat.players[0]->cards);

        /* iterate over player's cards */
        for (ptr = g_list_last(gskat.players[0]->cards); ptr; ptr = ptr->prev)
        {
            card = ptr->data;

            if ((gint) event->x >= card->dim.x
                    && (gint) event->y >= card->dim.y
                    && (gint) event->x < card->dim.x + card->dim.w
                    && (gint) event->y < card->dim.y + card->dim.h)
            {
                if (g_list_index(poss, card) == -1)
                {
                    /* set cross cursor if not already set */
                    if (!cursor || cursor->type != GDK_PIRATE)
                        gdk_window_set_cursor(window, gskat.pirate_cursor);

                    if (poss)
                        g_list_free(poss);

                    return FALSE;
                }
                else
                {
                    /* set hand cursor if not already set */
                    if (!cursor || cursor->type != GDK_HAND1)
                        gdk_window_set_cursor(window, gskat.hand_cursor);

                    if (poss)
                        g_list_free(poss);

                    return FALSE;
                }
            }
        }
    }

    /* reset to default cursor if necessary */
    if (cursor)
        gdk_window_set_cursor(window, NULL);

    if (poss)
        g_list_free(poss);

    return FALSE;
}

/**
   mouse_click:
 * @area:   #GtkDrawingArea triggering the event
 * @event:  #GdkEventButton structure
 * @data:   arbitrary user data
 *
 * Callback function of a mouse click event in the drawing area
 *
 * Check for the current game state and trigger the appropriate action
 *
 * Returns: %TRUE to not handle the event any further, otherwise %FALSE
 */
gboolean mouse_click(GtkWidget *area, GdkEventButton *event, gpointer data)
{
    UNUSED(area);
    UNUSED(data);

    gboolean found = FALSE;

    /* left mouse button click */
    if (event->button == 1)
    {
        if (gskat.state == TAKESKAT && gskat.re == gskat.players[0]
                && !gskat.hand)
            found = click_skat(event);
        else if (gskat.state == PLAYING)
            found = play_card(event);
        else if (gskat.state == READY)
        {
            gskat.state = PLAYING;
            calculate_stich();

            /* only continue playing if game was not ended early */
            if (gskat.state == PLAYING)
                play_stich();
            return TRUE;
        }
    }
    /* right mouse button click */
    else if (event->button == 3)
        /* show last trick(s) if activated in configuration */
        if (get_prop_bool("show_tricks"))
            show_last_tricks();

    return found;
}

/**
 * animation_toggle:
 * @tbutton:  #GtkCheckButton triggering the event
 * @data:     arbitrary user data
 *
 * Callback function of the 'animation' checkbox
 * in the config dialog
 */
void animation_toggle(GtkToggleButton *tbutton, gpointer data)
{
    gboolean active = gtk_toggle_button_get_active(tbutton);

    gtk_widget_set_sensitive((GtkWidget *) data, active);
}

/**
 * reaction_toggle:
 * @tbutton:  #GtkCheckButton triggering the event
 * @data:     arbitrary user data
 *
 * Callback function of the 'reaction' checkbox
 * in the config dialog
 */
void reaction_toggle(GtkToggleButton *tbutton, gpointer data)
{
    gboolean active = gtk_toggle_button_get_active(tbutton);

    gtk_widget_set_sensitive((GtkWidget *) data, active);
}
/**
 * show_tricks_toggle:
 * @tbutton:  #GtkCheckButton triggering the event
 * @data:     arbitrary user data
 *
 * Callback function of the 'animation' checkbox
 * in the config dialog
 */
void show_tricks_toggle(GtkToggleButton *tbutton, gpointer data)
{
    gboolean active = gtk_toggle_button_get_active(tbutton);

    gtk_widget_set_sensitive((GtkWidget *) data, active);
}

/**
 * refresh:
 * @area:   #GtkDrawingArea triggering the event
 * @event:  #GdkEventExpose structure
 * @data:   arbitrary user data
 *
 * Redraw the game area on the 'expose' signal
 */
void refresh(GtkWidget *area, GdkEventExpose *event, gpointer data)
{
    UNUSED(area);
    UNUSED(event);
    UNUSED(data);

    if (gskat.area && gskat.area->window)
        draw_area();
}

/**
 * refresh_tricks:
 * @area:   #GtkDrawingArea triggering the event
 * @event:  #GdkEventExpose structure
 * @data:   stich_view structure
 *
 * Redraw the @area in the show last tricks dialog window
 */
void refresh_tricks(GtkWidget *area, GdkEventExpose *event, gpointer data)
{
    UNUSED(event);

    draw_tricks_area(area, (stich_view *) data);
}

/**
 * gameload_cb:
 * @menuitem:  #GtkMenuItem which received the signal
 * @data:      arbitrary user data
 *
 * Load a saved game state
 */
void gameload_cb(GtkMenuItem *menuitem, gpointer data)
{
    UNUSED(menuitem);
    UNUSED(data);

    gchar *file;
    GtkWidget *file_chooser = gtk_file_chooser_dialog_new(
            _("Load game"),
            GTK_WINDOW(gskat.window),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OK, GTK_RESPONSE_OK,
            NULL);

    if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_OK)
    {
        file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));

        load_game(file);

        g_free(file);
    }

    gtk_widget_destroy(file_chooser);
}

/**
 * gamesave_cb:
 * @menuitem:  #GtkMenuItem which received the signal
 * @data:      arbitrary user data
 *
 * Save the current game state into a file
 */
void gamesave_cb(GtkMenuItem *menuitem, gpointer data)
{
    UNUSED(menuitem);
    UNUSED(data);

    gchar *file;
    GtkWidget *file_chooser = gtk_file_chooser_dialog_new(
            _("Save game"),
            GTK_WINDOW(gskat.window),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OK, GTK_RESPONSE_OK,
            NULL);

    if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_OK)
    {
        file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));

        save_state_to_file(file);

        g_free(file);
    }

    gtk_widget_destroy(file_chooser);
}

/**
 * quickload_game_cb:
 * @menuitem:  #GtkMenuItem which received the signal
 * @data:      arbitrary user data
 *
 * Load a saved game state
 */
void quickload_game_cb(GtkMenuItem *menuitem, gpointer data)
{
    UNUSED(menuitem);
    UNUSED(data);

    gchar *filename = g_build_filename(get_data_dir(), "gamestate", NULL);

    load_game(filename);

    g_free(filename);
}

/**
 * quicksave_game_cb:
 * @menuitem:  #GtkMenuItem which received the signal
 * @data:      arbitrary user data
 *
 * Save the current game state
 */
void quicksave_game_cb(GtkMenuItem *menuitem, gpointer data)
{
    UNUSED(menuitem);
    UNUSED(data);

    gchar *filename = g_build_filename(get_data_dir(), "gamestate", NULL);

    save_state_to_file(filename);

    g_free(filename);
}

/**
 * infobar_bid_response:
 * @ib:       #GtkInfoBar widget emitting the signal
 * @response: Response of the user
 * @data:     #gboolean hoeren cast to #gpointer
 *
 * Get the bidding response from the user
 */
void infobar_bid_response(GtkInfoBar *ib, gint response, gpointer data)
{
    gboolean hoeren = GPOINTER_TO_INT(data);

    gtk_widget_destroy(GTK_WIDGET(ib));

    do_player_bid(response, hoeren);
}

/**
 * player_draw_bid:
 * @data:  #player to draw the bidden value for
 *
 * Deactivate the drawn bid value and refresh the game area
 *
 * Returns: %FALSE to remove the timeout
 */
gboolean player_draw_bid(gpointer data)
{
    player *p = (player *) data;

    p->does_bid = FALSE;
    draw_area();

    return FALSE;
}

/* vim:set et sw=4 sts=4 tw=80: */
