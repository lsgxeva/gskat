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
#include "common.h"
#include "configuration.h"
#include "interface.h"

app gskat;

static gboolean debug        = FALSE;
static gboolean no_animation = FALSE;
static gboolean version_only = FALSE;
static gchar *data_dir = NULL;

static GOptionEntry arguments[] =
{
    {
        "data", 0, 0, G_OPTION_ARG_FILENAME, &data_dir,
        N_("card/image directory"), NULL
    },
    {
        "debug", 0, 0, G_OPTION_ARG_NONE, &debug,
        N_("toggle debug mode (for developing)"), NULL
    },
    {
        "no_animation", 0, 0, G_OPTION_ARG_NONE, &no_animation,
        N_("disable card animations"), NULL
    },
    {
        "version", 0, 0, G_OPTION_ARG_NONE, &version_only,
        N_("print version and exit"), NULL
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

/**
 * initialize:
 *
 * Initialize main game objects
 */
static void initialize()
{
    gskat.cards         = NULL;
    gskat.skat          = NULL;
    gskat.table         = NULL;
    gskat.players       = NULL;
    gskat.stiche        = NULL;
    gskat.player_names  = NULL;
    gskat.icons         = NULL;
    gskat.back          = NULL;
    gskat.bg            = NULL;
    gskat.area          = NULL;
    gskat.window        = NULL;
    gskat.widgets       = NULL;
    gskat.state         = LOADING;
    gskat.re            = NULL;
    gskat.forehand      = 2;
    gskat.cplayer       = -1;
    gskat.trump         = -1;
    gskat.round         = 1;
    gskat.stich         = 1;
    gskat.bidden        = 0;
    gskat.sager         = -1;
    gskat.hoerer        = -1;
    gskat.hand          = FALSE;
    gskat.null          = FALSE;
    gskat.hand_cursor   = NULL;
    gskat.pirate_cursor = NULL;
    gskat.log_level     = MT_INFO;
    gskat.log           = g_string_sized_new(256);
    gskat.datadir       = NULL;
    gskat.config        = NULL;
}

int main(int argc, const char *argv[])
{
    GOptionContext *context;
    GError *error = NULL;

    /* initialization of game objects */
    initialize();

    /* set locale environment */
#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, GSKAT_LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    /* parse command line arguments */
    context = g_option_context_new(_(" - GTK Skat"));

    if (context)
    {
        g_option_context_add_main_entries(context, arguments, GETTEXT_PACKAGE);
        g_option_context_add_group(context, gtk_get_option_group(TRUE));

        if (!g_option_context_parse(context, &argc, (gchar ***) &argv, &error))
        {
            gskat_msg(MT_ERROR,
                    _("Failed to parse arguments: %s\n"), error->message);
            g_clear_error(&error);

            return 1;
        }

        g_option_context_free(context);

        if (version_only)
        {
            g_print(_("gskat %s\n"), VERSION);
            return 0;
        }

        /* initialize configuration */
        init_config();
        set_default_config();
        gskat.datadir = data_dir;

        /* load configuration */
        load_config();

        /* disable card animation if desired */
        if (no_animation)
            set_bool_val("animation", FALSE);

        /* set game icons */
        set_icons();

        /* initialize interface */
        create_interface();

        /* show all widgets after being initialized */
        if (gskat.widgets != NULL)
        {
            gtk_widget_show_all(gskat.window);
            gtk_main();

            free_app();
        }
    }

    return 0;
}

/* vim:set et sw=4 sts=4 tw=80: */
