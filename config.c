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

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include "def.h"
#include "config.h"

void load_config(app *app)
{
    gchar *filename;

    /* get home directory */
    const gchar *home_dir = g_getenv("HOME");
    if (!home_dir)
        home_dir = g_get_home_dir();

    filename = g_strconcat(home_dir, "/.gskat/gskat.conf", NULL);

    /* try to find config file */
    if (filename && g_file_test(filename, G_FILE_TEST_EXISTS))
    {
        DPRINT(("Found config file '%s'\n", filename));

        read_config(app, filename);
    }
    else
    {
        DPRINT(("Failed to load config from '%s'\n", filename));
        DPRINT(("Using default settings instead.\n"));

        /* set default values */
        set_default_config(app);

        /* try to save config */
        if (create_conf_dir(app, home_dir))
            write_config(app, home_dir);
    }

    g_free(filename);
}

void alloc_config(app *app)
{
    app->conf = (config *) g_malloc(sizeof(config));

    if (app->conf)
    {
        app->conf->player_names = (gchar **) g_malloc(sizeof(gchar *) * 3);
        app->conf->gui = TRUE;
    }
}

void set_default_config(app *app)
{
    /* set player names */
    const gchar *user_name = g_getenv("USER");

    app->conf->player_names[0] = g_strdup(user_name ? user_name : "Player");
    app->conf->player_names[1] = g_strdup("Cuyo");
    app->conf->player_names[2] = g_strdup("Dozo");
}

gboolean write_config(app *app, const gchar *filename)
{
    gsize length;
    gboolean done = FALSE;
    gchar *key_file_content = NULL;
    GKeyFile *keys = NULL;

    keys = g_key_file_new();

    if (keys)
    {
        g_key_file_set_string_list(keys, "gskat", "player_names",
                (const gchar **) app->conf->player_names, 3);

        key_file_content = g_key_file_to_data(keys, &length, NULL);

        g_key_file_free(keys);
    }

    if (key_file_content)
    {
        done = g_file_set_contents(filename, key_file_content, length, NULL);

        if (done)
            DPRINT(("Saved configuration: %s\n", filename));
        else
            DPRINT(("Failed to save configuration: %s\n", filename));

        g_free(key_file_content);
    }

    return done;
}

gboolean read_config(app *app, const gchar *filename)
{
    gsize length;
    gboolean done = FALSE;
    GError *error = NULL;
    GKeyFile *keyfile = NULL;

    keyfile = g_key_file_new();

    if (keyfile)
    {
        done = g_key_file_load_from_file(keyfile, filename,
                G_KEY_FILE_NONE, &error);

        if (error)
        {
            DPRINT(("Error on reading configuration file: %s\n", error->message));

            g_clear_error(&error);
        }

        if (done)
        {
            /* read player names */
            app->conf->player_names = g_key_file_get_string_list(keyfile, "gskat",
                    "player_names", &length, NULL);
        }

        g_key_file_free(keyfile);
    }

    return done;
}

gboolean create_conf_dir(app *app, const gchar *home)
{
    gboolean done = FALSE, exists = FALSE;
    gchar *gtk_dir = g_strconcat(home, "/.gskat", NULL);

    if (gtk_dir)
    {
        exists = g_file_test(gtk_dir, G_FILE_TEST_EXISTS);

        if (!exists && g_mkdir(gtk_dir, 0755) != 0)
        {
            DPRINT(("Unable to create directory: %s\n", gtk_dir));
        }
        else
            done = TRUE;

        g_free(gtk_dir);
    }

    return done;
}

/* vim:set et sw=4 sts=4 tw=80: */