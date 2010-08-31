/*
 *  This file is part of gskat.
 *
 *  Copyright (C) 2010 by Gregor Uhlenheuer
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
#include "interface.h"
#include "utils.h"
#include "callback.h"

/**
 * @brief Allocate and initialize a new player structure
 *
 * @param id     player id
 * @param name   player name
 * @param human  TRUE if player is a human player, otherwise FALSE
 *
 * @return the new player object
 */
player *init_player(gint id, gchar *name, gboolean human)
{
    player *new = (player *) g_malloc(sizeof(player));

    if (new)
    {
        new->id = id;
        new->name = name;
        new->human = human;
        new->re = FALSE;
        new->points = 0;
        new->sum_points = 0;
        new->round_points = NULL;
        new->gereizt = 0;
        new->cards = NULL;
    }
    else
        g_printerr("Could not create new player %s\n", name);

    return new;
}

/**
 * @brief Try to load the icons of the four suits
 */
void load_icons()
{
    gint i;
    gchar *suits[] = { "club", "spade", "heart", "diamond" };
    gchar *filename;

    filename = (gchar *) g_malloc(sizeof(gchar) * (strlen(DATA_DIR) + 20));

    if (filename)
    {
        gskat.icons = (GdkPixbuf **) g_malloc(sizeof(GdkPixbuf *) * 4);

        for (i=0; i<4; i++)
        {
            g_sprintf(filename, "%s/icon-%s.xpm", DATA_DIR, suits[i]);

            DPRINT(("Loading '%s' ... ", filename));

            if (g_file_test(filename, G_FILE_TEST_EXISTS) == TRUE)
            {
                DPRINT(("OK\n"));
                gskat.icons[i] = gdk_pixbuf_new_from_file(filename, NULL);
            }
            else
            {
                DPRINT(("FAIL\n"));
                gskat.icons[i] = NULL;
            }
        }

        g_free(filename);
    }
}

/**
 * @brief Allocate all game objects like players, icons and stiche array
 */
void alloc_app()
{
    gint i;

    /* initialize players */
    gskat.players = (player **) g_malloc(sizeof(player *) * 3);

    if (gskat.players)
    {
        for (i=0; i<3; ++i)
            gskat.players[i] = init_player(i, gskat.conf.player_names[i],
                    i ? FALSE : TRUE);
    }
    else
        g_printerr("Could not create players.\n");

    /* initialize played cards */
    gskat.stiche = (card ***) g_malloc(sizeof(card **) * 10);

    for (i=0; i<10; ++i)
        gskat.stiche[i] = NULL;

    /* initialize suit icons */
    load_icons();

    /* initialize alternative cursor shapes */
    gskat.pirate_cursor = gdk_cursor_new(GDK_PIRATE);
    gskat.hand_cursor = gdk_cursor_new(GDK_HAND1);
}

/**
 * @brief Show a dialog window showing the last trick(s)
 */
void show_last_tricks()
{
    gint cur, x, y;
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *area;
    GtkWidget *hsep;
    GtkWidget *hbox_button;
    GtkWidget *prev_button;
    GtkWidget *next_button;
    GtkWidget *button;
    card **stich;
    stich_view *sv = NULL;

    cur = gskat.stich - 2;

    /* return if there is no stich to show */
    if (cur < 0 || (stich = gskat.stiche[cur]) == NULL)
        return;

    /* get minimal drawing area size to request */
    x = ((*gskat.stiche[0])->dim.w + 5) * 3 + 5;
    y = (*gskat.stiche[0])->dim.h + 80;

    /* dialog window widgets */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Letzter Stich");
    gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(window),
            GTK_WINDOW(gskat.allwidgets[0]));

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    /* drawing area */
    area = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(vbox), area, TRUE, TRUE, 2);
    gtk_widget_set_size_request(area, x, y);
    gtk_widget_set_double_buffered(area, TRUE);

    hsep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hsep, FALSE, FALSE, 0);

    hbox_button = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_button, FALSE, FALSE, 2);

    /* previous stich button */
    prev_button = gtk_button_new_from_stock(GTK_STOCK_GO_BACK);
    gtk_box_pack_start(GTK_BOX(hbox_button), prev_button, FALSE, FALSE, 2);

    /* deactivate previous button if the first stich is already shown
     * or if 'show_tricks' is turned off
     * or if 'num_show_tricks' == 1 */
    if (cur == 0 || !gskat.conf.show_tricks ||
            gskat.conf.num_show_tricks <= 1)
        gtk_widget_set_sensitive(prev_button, FALSE);

    /* close/ok button */
    button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_box_pack_start(GTK_BOX(hbox_button), button, FALSE, FALSE, 2);

    /* next stich button (initially deactivated) */
    next_button = gtk_button_new_from_stock(GTK_STOCK_GO_FORWARD);
    gtk_box_pack_start(GTK_BOX(hbox_button), next_button, FALSE, FALSE, 2);
    gtk_widget_set_sensitive(next_button, FALSE);

    /* initialize stich_view structure */
    if (!(sv = g_malloc(sizeof(stich_view))))
        return;

    sv->cur = cur;
    sv->stich = stich;
    sv->window = window;
    sv->area = area;
    sv->prevb = prev_button;
    sv->nextb = next_button;

    /* connect signals with callback functions */
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(close_show_trick), (gpointer) sv);
    g_signal_connect(G_OBJECT(prev_button), "clicked",
            G_CALLBACK(prev_stich_click), (gpointer) sv);
    g_signal_connect(G_OBJECT(next_button), "clicked",
            G_CALLBACK(next_stich_click), (gpointer) sv);
    g_signal_connect(G_OBJECT(area), "expose-event",
            G_CALLBACK(refresh_tricks), (gpointer) sv);
    g_signal_connect(G_OBJECT(window), "delete-event",
            G_CALLBACK(destroy_show_trick), (gpointer) sv);

    gtk_widget_show_all(window);

    draw_tricks_area(area, sv);
}

/**
 * @brief Show the configuration dialog window and initialize the
 * widgets with the current config values
 */
void show_config_window()
{
    gint i;
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *notebook;

    GtkWidget *names_label;
    GtkWidget *names_table;
    GtkWidget *player_label[3];
    GtkWidget *player_entry[3];

    GtkWidget *misc_label;
    GtkWidget *misc_table;
    GtkWidget *animation_label;
    GtkWidget *animation_check;
    GtkWidget *animation_dur_label;
    GtkWidget *animation_duration;
    GtkWidget *debug_label;
    GtkWidget *debug_check;

    GtkWidget *rules_label;
    GtkWidget *rules_table;
    GtkWidget *show_tricks_label;
    GtkWidget *show_tricks_check;
    GtkWidget *num_show_tricks_label;
    GtkWidget *num_show_tricks;

    GtkWidget *hbox_buttons;
    GtkWidget *ok_button;
    GtkWidget *cancel_button;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Einstellungen");
    gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    gtk_widget_set_size_request(window, 300, 200);
    gtk_window_set_transient_for(GTK_WINDOW(window),
            GTK_WINDOW(gskat.allwidgets[0]));

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    /* PLAYER NAMES TABLE */
    names_label = gtk_label_new("Spieler-Namen");
    names_table = gtk_table_new(3, 2, TRUE);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), names_table, names_label);

    player_label[0] = gtk_label_new("Spieler-Name:");
    player_label[1] = gtk_label_new("Spieler 1:");
    player_label[2] = gtk_label_new("Spieler 2:");

    for (i=0; i<3; ++i)
    {
        gtk_misc_set_alignment(GTK_MISC(player_label[i]), 0, 0.5);
        gtk_table_attach_defaults(GTK_TABLE(names_table),
                player_label[i],
                0, 1, i, i+1);

        player_entry[i] = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(player_entry[i]),
                gskat.conf.player_names[i]);
        gtk_table_attach_defaults(GTK_TABLE(names_table),
                player_entry[i],
                1, 2, i, i+1);
    }

    /* RULES TABLE */
    rules_label = gtk_label_new("Regeln");
    rules_table = gtk_table_new(2, 2, FALSE);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), rules_table, rules_label);

    show_tricks_label = gtk_label_new("Zeige letzten Stich:");
    gtk_misc_set_alignment(GTK_MISC(show_tricks_label), 0, 0.5);

    show_tricks_check = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_tricks_check),
            gskat.conf.show_tricks);
    g_signal_connect(G_OBJECT(show_tricks_check), "toggled",
            G_CALLBACK(show_tricks_toggle), NULL);

    gtk_table_attach(GTK_TABLE(rules_table),
            show_tricks_label,
            0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(rules_table),
            show_tricks_check,
            1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 10, 0);

    num_show_tricks_label = gtk_label_new("Anzahl letzter Stiche:");
    gtk_misc_set_alignment(GTK_MISC(num_show_tricks_label), 0, 0.5);

    num_show_tricks = gtk_spin_button_new_with_range(1, 11, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(num_show_tricks), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(num_show_tricks),
            gskat.conf.num_show_tricks);
    gtk_widget_set_sensitive(num_show_tricks, gskat.conf.show_tricks);

    gtk_table_attach(GTK_TABLE(rules_table),
            num_show_tricks_label,
            0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(rules_table),
            num_show_tricks,
            1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 10, 0);

    /* MISC TABLE */
    misc_label = gtk_label_new("Sonstiges");
    misc_table = gtk_table_new(3, 2, FALSE);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), misc_table, misc_label);

    /* animation */
    animation_label = gtk_label_new("Animiere Kartenbewegung:");
    gtk_misc_set_alignment(GTK_MISC(animation_label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(misc_table),
            animation_label,
            0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    animation_check = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(animation_check),
            gskat.conf.animation);
    gtk_table_attach(GTK_TABLE(misc_table),
            animation_check,
            1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 10, 0);

    /* animation duration */
    animation_dur_label = gtk_label_new("Animationsdauer:");
    gtk_misc_set_alignment(GTK_MISC(animation_dur_label), 0, 0.5);
    gtk_widget_set_sensitive(animation_dur_label, gskat.conf.animation);
    gtk_table_attach(GTK_TABLE(misc_table),
            animation_dur_label,
            0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    animation_duration = gtk_spin_button_new_with_range(25, 5000, 10.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(animation_duration), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(animation_duration),
            gskat.conf.anim_duration);
    gtk_widget_set_sensitive(animation_duration, gskat.conf.animation);
    gtk_table_attach(GTK_TABLE(misc_table),
            animation_duration,
            1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 10, 0);

    g_signal_connect(G_OBJECT(animation_check), "toggled",
            G_CALLBACK(animation_toggle), NULL);

    /* debugging */
    debug_label = gtk_label_new("Debug Meldungen:");
    gtk_misc_set_alignment(GTK_MISC(debug_label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(misc_table),
            debug_label,
            0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    debug_check = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(debug_check),
            gskat.conf.debug);
    gtk_table_attach(GTK_TABLE(misc_table),
            debug_check,
            1, 2, 2, 3, GTK_SHRINK, GTK_SHRINK, 10, 0);

#ifdef DEBUG
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(debug_check), TRUE);

    gtk_widget_set_sensitive(debug_label, FALSE);
    gtk_widget_set_sensitive(debug_check, FALSE);
#endif

    /* BOTTOM BUTTONS */
    hbox_buttons = gtk_hbox_new(TRUE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);

    ok_button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    g_signal_connect(G_OBJECT(ok_button), "clicked",
            G_CALLBACK(save_config), (gpointer) window);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), ok_button, FALSE, FALSE, 0);

    cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(G_OBJECT(cancel_button), "clicked",
            G_CALLBACK(close_config), (gpointer) window);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), cancel_button, FALSE, FALSE, 0);

    /* allocate configuration widgets
     * this array is freed in either 'save_config', 'close_config'
     * or 'destroy_config' */
    gskat.confwidgets = (GtkWidget **) g_malloc(8 * sizeof(GtkWidget *));

    gskat.confwidgets[0] = player_entry[0];
    gskat.confwidgets[1] = player_entry[1];
    gskat.confwidgets[2] = player_entry[2];
    gskat.confwidgets[3] = animation_check;
    gskat.confwidgets[4] = animation_duration;
    gskat.confwidgets[5] = debug_check;
    gskat.confwidgets[6] = show_tricks_check;
    gskat.confwidgets[7] = num_show_tricks;

    g_signal_connect(G_OBJECT(window), "delete-event",
            G_CALLBACK(destroy_config), (gpointer) window);

    gtk_widget_show_all(window);
}

/**
 * @brief Create the main menu and populate it with the menu items
 *
 * @return the new main menu GtkWidget
 */
static GtkWidget *create_menu()
{
    GtkWidget *menu;         /* main menu */
    GtkWidget *gmenu;        /* game submenu */
    GtkWidget *game;
    GtkWidget *new_item;
    GtkWidget *quit_item;
    GtkWidget *cmenu;        /* configuration submenu */
    GtkWidget *config;
    GtkWidget *options_item;
    GtkWidget *hmenu;        /* help submenu */
    GtkWidget *help;
    GtkWidget *about_item;

    menu = gtk_menu_bar_new();

    /* game submenu */
    new_item = gtk_menu_item_new_with_label("Neue Runde");
    g_signal_connect(G_OBJECT(new_item), "activate",
            G_CALLBACK(next_round), NULL);
    quit_item = gtk_menu_item_new_with_label("Beenden");
    g_signal_connect(G_OBJECT(quit_item), "activate", G_CALLBACK(quit), NULL);

    gmenu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(gmenu), new_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(gmenu), quit_item);

    game = gtk_menu_item_new_with_label("Spiel");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(game), gmenu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), game);

    /* configuration submenu */
    options_item = gtk_menu_item_new_with_label("Optionen");
    g_signal_connect(G_OBJECT(options_item), "activate",
            G_CALLBACK(show_config_window), NULL);

    cmenu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(cmenu), options_item);

    config = gtk_menu_item_new_with_label("Einstellungen");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(config), cmenu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), config);

    /* help submenu */
    about_item = gtk_menu_item_new_with_label("Über");

    hmenu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(hmenu), about_item);

    help = gtk_menu_item_new_with_label("Hilfe");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help), hmenu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), help);

    return menu;
}

/**
 * @brief Create and allocate the main window layout
 */
void create_interface()
{
    GtkWidget *window;
    GtkWidget *vboxmenu;
    GtkWidget *mainmenu;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *area;
    GtkWidget *frame_game;
    GtkWidget *table_game;
    GtkWidget *lb_game_stich_left;
    GtkWidget *lb_game_re_left;
    GtkWidget *lb_game_spiel_left;
    GtkWidget *lb_game_gereizt_left;
    GtkWidget *lb_game_stich_right;
    GtkWidget *lb_game_re_right;
    GtkWidget *lb_game_spiel_right;
    GtkWidget *lb_game_gereizt_right;
    GtkWidget *frame_rank;
    GtkWidget *scrolled_win;
    GtkWidget *vbox_table;
    GtkWidget *table_rank;
    GtkWidget *table_points;
    GtkWidget *hsep;
    GtkWidget *lb_rank_p1_name;
    GtkWidget *lb_rank_p2_name;
    GtkWidget *lb_rank_p3_name;
    GtkWidget *lb_rank_p1;
    GtkWidget *lb_rank_p2;
    GtkWidget *lb_rank_p3;
    GtkWidget *button;

    gchar *iconfile = (gchar *) g_malloc(sizeof(gchar) * strlen(DATA_DIR)+20);

    if (iconfile)
        g_sprintf(iconfile, "%s/gskat.png", DATA_DIR);

    gskat.allwidgets = (GtkWidget **) g_malloc(sizeof(GtkWidget *) * 14);

    if (gskat.allwidgets != NULL)
    {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "gskat");

        if (g_file_test(iconfile, G_FILE_TEST_EXISTS))
            gtk_window_set_icon_from_file(GTK_WINDOW(window), iconfile, NULL);

        g_free(iconfile);

        vboxmenu = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(window), vboxmenu);

        mainmenu = create_menu();
        if (mainmenu)
            gtk_box_pack_start(GTK_BOX(vboxmenu), mainmenu, FALSE, FALSE, 0);

        hbox = gtk_hbox_new(FALSE, 0);
        /* gtk_box_pack_start(child, expand, fill, padding) */
        gtk_box_pack_start(GTK_BOX(vboxmenu), hbox, TRUE, TRUE, 0);

        area = gtk_drawing_area_new();
        gtk_box_pack_start(GTK_BOX(hbox), area, TRUE, TRUE, 2);
        gtk_widget_set_size_request(area, 450, 500);
        gtk_widget_set_double_buffered(area, TRUE);

        vbox = gtk_vbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 2);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);

        frame_game = gtk_frame_new("Runde 1");
        gtk_frame_set_label_align(GTK_FRAME(frame_game), 0.5, 0.5);
        gtk_box_pack_start(GTK_BOX(vbox), frame_game, FALSE, TRUE, 2);
        gtk_frame_set_shadow_type(GTK_FRAME(frame_game), GTK_SHADOW_ETCHED_IN);

        /* gtk_table_new(rows, columns, homogeneous) */
        table_game = gtk_table_new(4, 2, FALSE);
        gtk_container_add(GTK_CONTAINER(frame_game), table_game);
        gtk_container_set_border_width(GTK_CONTAINER(table_game), 20);
        gtk_table_set_col_spacings(GTK_TABLE(table_game), 20);
        gtk_table_set_row_spacings(GTK_TABLE(table_game), 5);

        lb_game_stich_left = gtk_label_new("Stich:");
        lb_game_re_left = gtk_label_new("Re:");
        lb_game_spiel_left = gtk_label_new("Spiel:");
        lb_game_gereizt_left = gtk_label_new("Gereizt:");

        /* gtk_misc_set_alignment(misc, xalign, yalign)
         * xalign: 0 (left) to 1 (right)
         * yalign: 0 (top) to 1 (bottom) */
        gtk_misc_set_alignment(GTK_MISC(lb_game_stich_left), 1, 0.5);
        gtk_misc_set_alignment(GTK_MISC(lb_game_re_left), 1, 0.5);
        gtk_misc_set_alignment(GTK_MISC(lb_game_spiel_left), 1, 0.5);
        gtk_misc_set_alignment(GTK_MISC(lb_game_gereizt_left), 1, 0.5);

        /* gtk_table_attach_defaults(parent, child, left, right, top, bottom) */
        gtk_table_attach_defaults(GTK_TABLE(table_game),
                lb_game_stich_left,
                0, 1, 0, 1);
        gtk_table_attach_defaults(GTK_TABLE(table_game),
                lb_game_re_left,
                0, 1, 1, 2);
        gtk_table_attach_defaults(GTK_TABLE(table_game),
                lb_game_spiel_left,
                0, 1, 2, 3);
        gtk_table_attach_defaults(GTK_TABLE(table_game),
                lb_game_gereizt_left,
                0, 1, 3, 4);

        lb_game_stich_right = gtk_label_new("1");
        lb_game_re_right = gtk_label_new("-");
        lb_game_spiel_right = gtk_label_new("-");
        lb_game_gereizt_right = gtk_label_new("-");

        gtk_misc_set_alignment(GTK_MISC(lb_game_stich_right), 1, 0.5);
        gtk_misc_set_alignment(GTK_MISC(lb_game_re_right), 1, 0.5);
        gtk_misc_set_alignment(GTK_MISC(lb_game_spiel_right), 1, 0.5);
        gtk_misc_set_alignment(GTK_MISC(lb_game_gereizt_right), 1, 0.5);

        gtk_table_attach_defaults(GTK_TABLE(table_game),
                lb_game_stich_right,
                1, 2, 0, 1);
        gtk_table_attach_defaults(GTK_TABLE(table_game),
                lb_game_re_right,
                1, 2, 1, 2);
        gtk_table_attach_defaults(GTK_TABLE(table_game),
                lb_game_spiel_right,
                1, 2, 2, 3);
        gtk_table_attach_defaults(GTK_TABLE(table_game),
                lb_game_gereizt_right,
                1, 2, 3, 4);

        /* game rankings */
        frame_rank = gtk_frame_new("Spielstand");
        gtk_frame_set_label_align(GTK_FRAME(frame_rank), 0.5, 0.5);
        gtk_box_pack_start(GTK_BOX(vbox), frame_rank, TRUE, TRUE, 2);
        gtk_frame_set_shadow_type(GTK_FRAME(frame_rank), GTK_SHADOW_ETCHED_IN);

        scrolled_win = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scrolled_win), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_win),
                GTK_SHADOW_NONE);
        gtk_container_add(GTK_CONTAINER(frame_rank), scrolled_win);

        vbox_table = gtk_vbox_new(FALSE, 2);
        gtk_container_set_border_width(GTK_CONTAINER(vbox_table), 5);

        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win),
                vbox_table);

        /* players' points table */
        table_rank = gtk_table_new(1, 3, TRUE);
        gtk_box_pack_start(GTK_BOX(vbox_table), table_rank, FALSE, TRUE, 2);
        gtk_container_set_border_width(GTK_CONTAINER(table_rank), 5);
        gtk_table_set_col_spacings(GTK_TABLE(table_rank), 10);
        gtk_table_set_row_spacings(GTK_TABLE(table_rank), 5);

        lb_rank_p1_name = gtk_label_new(gskat.conf.player_names[0]);
        lb_rank_p2_name = gtk_label_new(gskat.conf.player_names[1]);
        lb_rank_p3_name = gtk_label_new(gskat.conf.player_names[2]);

        gtk_table_attach_defaults(GTK_TABLE(table_rank),
                lb_rank_p1_name,
                0, 1, 0, 1);
        gtk_table_attach_defaults(GTK_TABLE(table_rank),
                lb_rank_p2_name,
                1, 2, 0, 1);
        gtk_table_attach_defaults(GTK_TABLE(table_rank),
                lb_rank_p3_name,
                2, 3, 0, 1);

        hsep = gtk_hseparator_new();
        gtk_box_pack_start(GTK_BOX(vbox_table), hsep, FALSE, TRUE, 0);

        table_points = gtk_table_new(1, 3, TRUE);
        gtk_container_set_border_width(GTK_CONTAINER(table_points), 5);
        gtk_table_set_col_spacings(GTK_TABLE(table_points), 10);
        gtk_table_set_row_spacings(GTK_TABLE(table_points), 0);

        lb_rank_p1 = gtk_label_new("");
        lb_rank_p2 = gtk_label_new("");
        lb_rank_p3 = gtk_label_new("");

        gtk_label_set_markup(GTK_LABEL(lb_rank_p1), "<b>0</b>");
        gtk_label_set_markup(GTK_LABEL(lb_rank_p2), "<b>0</b>");
        gtk_label_set_markup(GTK_LABEL(lb_rank_p3), "<b>0</b>");

        gtk_table_attach_defaults(GTK_TABLE(table_points),
                lb_rank_p1,
                0, 1, 0, 1);
        gtk_table_attach_defaults(GTK_TABLE(table_points),
                lb_rank_p2,
                1, 2, 0, 1);
        gtk_table_attach_defaults(GTK_TABLE(table_points),
                lb_rank_p3,
                2, 3, 0, 1);

        gtk_box_pack_start(GTK_BOX(vbox_table), table_points, FALSE, TRUE, 2);

        button = gtk_button_new_with_label("Neue Runde");
        gtk_widget_set_sensitive(button, FALSE);
        gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 2);

        /* set game object pointers */
        gskat.area = area;

        gskat.allwidgets[0] = window;
        gskat.allwidgets[1] = button;
        gskat.allwidgets[2] = lb_game_stich_right;
        gskat.allwidgets[3] = lb_game_re_right;
        gskat.allwidgets[4] = lb_game_spiel_right;
        gskat.allwidgets[5] = lb_game_gereizt_right;
        gskat.allwidgets[6] = lb_rank_p1;
        gskat.allwidgets[7] = lb_rank_p2;
        gskat.allwidgets[8] = lb_rank_p3;
        gskat.allwidgets[9] = frame_game;
        gskat.allwidgets[10] = table_rank;
        gskat.allwidgets[11] = lb_rank_p1_name;
        gskat.allwidgets[12] = lb_rank_p2_name;
        gskat.allwidgets[13] = lb_rank_p3_name;

        /* attach signals */
        g_signal_connect(G_OBJECT(window), "destroy",
                G_CALLBACK(quit), NULL);
        g_signal_connect(G_OBJECT(area), "realize",
                G_CALLBACK(realization), NULL);
        g_signal_connect(G_OBJECT(area), "configure_event",
                G_CALLBACK(configure), NULL);
        g_signal_connect(G_OBJECT(area), "expose_event",
                G_CALLBACK(refresh), NULL);
        g_signal_connect(G_OBJECT(button), "clicked",
                G_CALLBACK(next_round), NULL);

        gtk_widget_add_events(area, GDK_BUTTON_PRESS_MASK |
                GDK_POINTER_MOTION_MASK);
        g_signal_connect(G_OBJECT(area), "button_press_event",
                G_CALLBACK(mouse_click), NULL);
        g_signal_connect(G_OBJECT(area), "motion-notify-event",
                G_CALLBACK(mouse_move), NULL);
    }
}

/**
 * @brief Update the players' points on the right-hand interface
 */
void update_rank_interface()
{
    gint i, len = 0;
    gchar msg[128];
    player *cur;
    GtkWidget *rank_label;
    GtkTable *table = GTK_TABLE(gskat.allwidgets[10]);

    /* update sum of points */
    g_sprintf(msg, "<b>%d</b>", gskat.players[0]->sum_points);
    gtk_label_set_markup(GTK_LABEL(gskat.allwidgets[6]), msg);
    g_sprintf(msg, "<b>%d</b>", gskat.players[1]->sum_points);
    gtk_label_set_markup(GTK_LABEL(gskat.allwidgets[7]), msg);
    g_sprintf(msg, "<b>%d</b>", gskat.players[2]->sum_points);
    gtk_label_set_markup(GTK_LABEL(gskat.allwidgets[8]), msg);

    /* get the number of rows */
    g_object_get(G_OBJECT(table), "n-rows", &len, NULL);

    /* add a new row in the table_rank list */
    gtk_table_resize(table, len+1, 3);

    for (i=0; i<3; ++i)
    {
        cur = gskat.players[i];

        /* get last entry in round_points list */
        g_sprintf(msg, "%d", GPOINTER_TO_INT(g_list_nth_data(cur->round_points,
                        g_list_length(cur->round_points)-1)));
        rank_label = gtk_label_new(msg);

        gtk_table_attach_defaults(table, rank_label, i, i+1, len, len+1);
        gtk_widget_show_all(GTK_WIDGET(table));
    }
}

/**
 * @brief Load the card image of the given rank and suit
 *
 * @param list  list to put the loaded card into
 * @param file  file to load the card image from
 * @param rank  card rank
 * @param suit  card suit
 */
void load_card(GList **list, const gchar *file, gint rank, gint suit)
{
    card *tcard = (card *) g_malloc(sizeof(card));

    if (tcard != NULL)
    {
        tcard->rank = rank;
        tcard->suit = suit;
        tcard->owner = -1;

        tcard->points = get_card_points(rank);

        tcard->dim.x = 0;
        tcard->dim.y = 0;

        tcard->draw = FALSE;

        tcard->img = cairo_image_surface_create_from_png(file);
        tcard->dim.w = cairo_image_surface_get_width(tcard->img);
        tcard->dim.h = cairo_image_surface_get_height(tcard->img);

        *list = g_list_prepend(*list, (gpointer) tcard);
    }
}

/**
 * @brief Allocate and initialize all 32 game cards
 *
 * @param path  path to the card image files
 *
 * @return TRUE on success, otherwise FALSE
 */
gboolean load_cards(const gchar *path)
{
    gint i, j, id, max = strlen(path)+30;
    GList **list = &(gskat.cards);
    gint ranks[] = { 1, 7, 8, 9, 10, 11, 12, 13 };
    gboolean error = FALSE;

    gchar *cname = (gchar *) g_malloc(sizeof(gchar) * max);

    /* create all 32 cards */
    if (cname != NULL)
    {
        for (i=0; i<4; ++i)
        {
            for (j=0; j<8; ++j)
            {
                id = SUITS[i] + ranks[j];
                g_sprintf(cname, "%s/%d.png", path, id);

                DPRINT(("Loading '%s' ... ", cname));

                if (g_file_test(cname, G_FILE_TEST_EXISTS))
                {
                    load_card(list, cname, ranks[j], SUITS[i]);
                    DPRINT(("OK\n"));
                }
                else
                {
                    error = TRUE;
                    DPRINT(("FAIL\n"));
                }
            }
        }
    }

    /* load back of cards */
    g_sprintf(cname, "%s/back.png", path);
    gskat.back = load_image(cname);

    /* load back of cards */
    g_sprintf(cname, "%s/bg.png", path);
    gskat.bg = load_image(cname);

    g_free(cname);

    return !error;
}

/**
 * @brief Load an image and create a new cairo surface on its basis
 *
 * @param filename  filename of the card image to load
 */
cairo_surface_t *load_image(gchar *filename)
{
    DPRINT(("Loading '%s' ... ", filename));

    if (g_file_test(filename, G_FILE_TEST_EXISTS))
    {
        DPRINT(("OK\n"));
        return cairo_image_surface_create_from_png(filename);
    }
    else
    {
        DPRINT(("FAIL\n"));
        return NULL;
    }
}

/**
 * @brief Position the player's cards on the game table
 *
 * @param player  player to position his cards for
 * @param x       x coordinate of the starting position
 * @param y       y coordinate of the starting position
 * @param step    offset between the cards in x direction
 */
void pos_player_cards(player *player, gint x, gint y, gint step)
{
    GList *ptr = NULL;
    card *card = NULL;

    if (player->cards)
    {
        for (ptr = g_list_first(player->cards); ptr; ptr = ptr->next)
        {
            card = ptr->data;
            card->dim.y = y;
            card->dim.x = x;
            x += step;
        }
    }
}

/**
 * @brief Calculate the card positions of all three players
 * based on the game window's dimension
 */
void calc_card_positions()
{
    gint x, y, win_w, win_h, card_w, card_h, step;
    GList *ptr = NULL;
    card *card = NULL;
    player *player = NULL;

    if ((ptr = g_list_first(gskat.cards)) && gskat.players)
    {
        card = ptr->data;
        card_w = card->dim.w;
        card_h = card->dim.h;

        win_w = gskat.area->allocation.width;
        win_h = gskat.area->allocation.height;

        /* player 0 */
        if ((player = gskat.players[0]))
        {
            if (player->cards && g_list_length(player->cards) > 0)
            {
                step = (win_w - card_w*1.5) / g_list_length(player->cards);
                y = win_h - (card_h + 5);
                x = card_w / 2;

                pos_player_cards(player, x, y, step);
            }
        }

        /* player 1 */
        if ((player = gskat.players[1]))
        {
            step = 10;
            y = 5;
            x = 5;

            pos_player_cards(player, x, y, step);
        }

        /* player 2 */
        if ((player = gskat.players[2]))
        {
            step = -10;
            y = 5;
            x = win_w - (card_w + 5);

            pos_player_cards(player, x, y, step);
        }

        /* cards in skat */
        if (gskat.skat && g_list_length(gskat.skat) > 0)
        {
            y = win_h / 2 - card_h / 2;
            x = win_w / 2 - card_w;
            step = card_w + 5;

            ptr = g_list_first(gskat.skat);
            card = ptr->data;
            card->dim.x = x;
            card->dim.y = y;
            x += step;

            ptr = g_list_last(gskat.skat);
            card = ptr->data;
            card->dim.x = x;
            card->dim.y = y;
        }

        /* cards on the table */
        if (gskat.table && g_list_length(gskat.table) > 0)
        {
            for (ptr = g_list_first(gskat.table); ptr; ptr = ptr->next)
            {
                card = ptr->data;

                /* do not update card position while moving */
                if (card->status != CS_MOVING)
                    set_table_position(card, &card->dim.x, &card->dim.y);
            }
        }
    }
}

/**
 * @brief Set the position of the given card on the game table
 *
 * @param card    card to set the table position for
 * @param dest_x  x coordinate of the destination
 * @param dest_y  y coordinate of the destination
 */
void set_table_position(card *card, gint *dest_x, gint *dest_y)
{
    gint card_w = card->dim.w;
    gint card_h = card->dim.h;
    gint win_w = gskat.area->allocation.width;
    gint win_h = gskat.area->allocation.height;

    gint y = win_h / 2 - card_h / 2;
    gint x = win_w / 2 - card_w / 2;

    if (card->owner == 0)
    {
        *dest_x = x;
        *dest_y = y + card_h / 3;
    }
    else if (card->owner == 1)
    {
        *dest_x = x - card_w / 3;
        *dest_y = y;
    }
    else if (card->owner == 2)
    {
        *dest_x = x + card_w / 3;
        *dest_y = y - card_h / 3;
    }
}

/**
 * @brief Calculate the card movement step
 *
 * Calculate the card movement step depending on the configuration value
 * 'anim_duration', the drawing timeout interval of 25 ms and the card
 * distance to move in x and y direction.
 *
 * @param cm  card_movement structure
 */
void set_card_move_step(card_move *cm)
{
    card *ptr = cm->mcard;

    gint dx = abs(ptr->dim.x - cm->dest_x);
    gint dy = abs(ptr->dim.y - cm->dest_y);

    cm->x_move = (gdouble) dx / 25;
    cm->y_move = (gdouble) dy / 25;

    if (!cm->x_move)
        cm->x_move = 1;

    if (!cm->y_move)
        cm->y_move = 1;
}

/**
 * @brief Move the given card towards its destination by the given
 * movement step
 *
 * The card movement structure contains the information about what card to move,
 * the movement destination and the movement step.
 *
 * @param data  card_move structure that contains the card movement information
 *
 * @return FALSE if the card movement is finished, otherwise TRUE
 */
gboolean move_card(gpointer data)
{
    card_move *cm = (card_move *) data;
    card *ptr = cm->mcard;

    gint step;
    gint x_move = cm->x_move;
    gint y_move = cm->y_move;
    gint dx = ptr->dim.x - cm->dest_x;
    gint dy = ptr->dim.y - cm->dest_y;

    /* adjust x coordinate */
    if (abs(dx) < x_move)
        ptr->dim.x = cm->dest_x;
    else
    {
        step = (dx > 0) ? -x_move : x_move;
        ptr->dim.x += step;
    }

    /* adjust y coordinate */
    if (abs(dy) < y_move)
        ptr->dim.y = cm->dest_y;
    else
    {
        step = (dy > 0) ? -y_move : y_move;
        ptr->dim.y += step;
    }

    /* check for finished movement */
    if (ptr->dim.x == cm->dest_x && ptr->dim.y == cm->dest_y)
    {
        ptr->status = CS_AVAILABLE;
        draw_area();

        g_free(cm);
        return FALSE;
    }

    draw_area();

    return TRUE;
}

/**
 * @brief Draw the cards from the last to the first
 *
 * @param cards   list of cards to draw
 * @param target  cairo drawing object
 */
void draw_cards(GList *cards, cairo_t *target)
{
    card *card;
    GList *ptr;
    cairo_surface_t *img = NULL;

    for (ptr = g_list_first(cards); ptr; ptr = ptr->next)
    {
        card = ptr->data;

        if (card && card->draw)
        {
            if (!card->draw_face)
                img = gskat.back;
            else
                img = card->img;
        }

        if (img)
        {
            cairo_set_source_surface(target, img, card->dim.x, card->dim.y);
            cairo_paint(target);
        }
    }
}

/**
 * @brief Draw the player's name on the game surface
 *
 * The player in the forehand position is drawn in red and
 * the others in black.
 *
 * @param player  player to draw the name of
 * @param cr      cairo drawing object
 */
void draw_player(player *player, cairo_t *cr)
{
    gchar *name;
    gint card_w, card_h, w, h;
    card *card = NULL;

    /* get screen dimensions */
    w = gskat.area->allocation.width;
    h = gskat.area->allocation.height;

    /* get card dimensions */
    card = g_list_nth_data(gskat.cards, 0);
    card_w = card->dim.w;
    card_h = card->dim.h;

    /* set font color and size */
    if (player->id == gskat.forehand)
        cairo_set_source_rgb(cr, 1.0, 0.1, 0.1);
    else
        cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);

    cairo_select_font_face(cr, "sans-serif",
            CAIRO_FONT_SLANT_NORMAL,
            CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 12);

    /* set name */
    name = (gchar *) g_malloc(sizeof(gchar) * strlen(player->name) + 6);

    if (player == gskat.re)
        g_sprintf(name, "%s (Re)", player->name);
    else
        g_sprintf(name, "%s", player->name);

    /* position text */
    if (player->id == 0)
        cairo_move_to(cr, w/2-strlen(name)*5, h-card_h-10);
    else if (player->id == 1)
        cairo_move_to(cr, 20, card_h+20);
    else
        cairo_move_to(cr, w-strlen(name)*10, card_h+20);

    /* draw text */
    cairo_show_text(cr, name);

    g_free(name);
}

/**
 * @brief Draw the table background
 *
 * @param area  GtkDrawingArea widget the background is drawn on
 * @param cr    Cairo drawing object
 */
void draw_table(GtkWidget *area, cairo_t *cr)
{
    cairo_pattern_t *pat = NULL;

    if (gskat.bg)
    {
        pat = cairo_pattern_create_for_surface(gskat.bg);
        cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);

        cairo_set_source(cr, pat);

    }
    else
        cairo_set_source_rgb(cr, 0, 0, 0);

    cairo_rectangle(cr, 0, 0,
            area->allocation.width,
            area->allocation.height);
    cairo_fill(cr);

    if (pat)
        cairo_pattern_destroy(pat);
}

/**
 * @brief Draw the game area with its players and their cards
 */
void draw_area()
{
    gint i;
    cairo_t *cr;
    player *player;

    GdkRectangle rect =
    {
        0, 0,
        gskat.area->allocation.width,
        gskat.area->allocation.height
    };

    gdk_window_begin_paint_rect(gskat.area->window, &rect);

    cr = gdk_cairo_create(gskat.area->window);

    draw_table(gskat.area, cr);

    if (gskat.skat)
        draw_cards(gskat.skat, cr);

    if (gskat.table)
        draw_cards(gskat.table, cr);

    if (gskat.players)
    {
        for (i=0; i<3; ++i)
        {
            player = gskat.players[i];
            draw_cards(player->cards, cr);
            draw_player(player, cr);
        }
    }

    cairo_destroy(cr);

    gdk_window_end_paint(gskat.area->window);
}

/**
 * @brief Draw the tricks in the show last tricks dialog window
 *
 * @param area   GtkDrawingArea widget the cards are drawn on
 * @param stich  stich (three cards) to draw
 */
void draw_tricks_area(GtkWidget *area, stich_view *sv)
{
    gint i, x, y;
    gchar *caption = NULL;
    cairo_t *cr;
    card **stich = sv->stich;

    GdkRectangle rect =
    {
        0, 0,
        area->allocation.width,
        area->allocation.height
    };

    gdk_window_begin_paint_rect(area->window, &rect);

    cr = gdk_cairo_create(area->window);

    /* draw table background */
    draw_table(area, cr);

    /* draw current stich */
    caption = g_strdup_printf("Stich %d", sv->cur + 1);

    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    cairo_select_font_face(cr, "sans-serif",
            CAIRO_FONT_SLANT_NORMAL,
            CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, area->allocation.width / 2 - 25, 25);
    cairo_show_text(cr, caption);

    /* draw cards of the given stich */
    x = 5;
    y = 40;

    for (i=0; i<3; ++i)
    {
        if (stich[i])
        {
            /* draw card image */
            cairo_set_source_surface(cr, stich[i]->img, x, y);
            cairo_paint(cr);

            /* draw card owner's name */
            cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
            cairo_select_font_face(cr, "sans-serif",
                    CAIRO_FONT_SLANT_NORMAL,
                    CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 12);
            cairo_move_to(cr, x, y + stich[i]->dim.h + 15);
            cairo_show_text(cr, gskat.players[stich[i]->owner]->name);

            x += stich[i]->dim.w + 5;
        }
    }

    cairo_destroy(cr);
    g_free(caption);

    gdk_window_end_paint(area->window);
}

/**
 * @brief Free all allocated in-game memory
 */
void free_app()
{
    GList *ptr;
    gint i;
    card *card;

    /* free players */
    for (i=0; i<3; ++i)
    {
        g_list_free(gskat.players[i]->cards);
        g_free(gskat.players[i]);
        gskat.players[i] = NULL;
    }
    g_free(gskat.players);
    gskat.players = NULL;

    /* free cards */
    for (ptr = g_list_first(gskat.cards); ptr; ptr = ptr->next)
    {
        card = ptr->data;
        if (card && card->img)
            cairo_surface_destroy(card->img);
        g_free(card);
        card = NULL;
    }
    g_list_free(gskat.cards);
    gskat.cards = NULL;
    g_list_free(gskat.skat);
    gskat.skat = NULL;
    g_list_free(gskat.played);
    gskat.played = NULL;

    /* free player names */
    g_strfreev(gskat.conf.player_names);
    gskat.conf.player_names = NULL;

    g_free(gskat.conf.filename);
    gskat.conf.filename = NULL;

    /* free played stiche */
    for (i=0; i<10; ++i)
    {
        if (gskat.stiche[i])
            g_free(gskat.stiche[i]);
        gskat.stiche[i] = NULL;
    }
    g_free(gskat.stiche);
    gskat.stiche = NULL;

    /* free icons */
    if (gskat.icons)
    {
        for (i=0; i<4; i++)
        {
            g_object_unref(gskat.icons[i]);
            gskat.icons[i] = NULL;
        }
        g_free(gskat.icons);
    }

    /* free remaining objects */
    if (gskat.back)
        cairo_surface_destroy(gskat.back);
    gskat.back = NULL;
    if (gskat.bg)
        cairo_surface_destroy(gskat.bg);
    gskat.bg = NULL;

    gdk_cursor_unref(gskat.pirate_cursor);
    gdk_cursor_unref(gskat.hand_cursor);

    g_free(gskat.allwidgets);

    DPRINT(("Quit gskat\n"));
}

/* vim:set et sw=4 sts=4 tw=80: */
