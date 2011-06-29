/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Red Hat, Inc.
 * Copyright (C) 2010 Intel, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include "cc-appearance-panel.h"
#include "appearance.h"
#include "appearance-desktop.h"
#include "appearance-font.h"
#include "appearance-themes.h"
#include "appearance-style.h"
#include "theme-installer.h"
#include "theme-thumbnail.h"
#include "activate-settings-daemon.h"
#include "capplet-util.h"

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib/gi18n-lib.h>

#include <libgnomeui/gnome-desktop-thumbnail.h>

#define CC_APPEARANCE_PANEL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_APPEARANCE_PANEL, CcAppearancePanelPrivate))

#define WID(s) GTK_WIDGET (gtk_builder_get_object (builder, s))

struct CcAppearancePanelPrivate
{
        GtkWidget *notebook;
        AppearanceData *data;
};

enum {
        PROP_0,
};

static void     cc_appearance_panel_class_init     (CcAppearancePanelClass *klass);
static void     cc_appearance_panel_init           (CcAppearancePanel      *appearance_panel);
static void     cc_appearance_panel_finalize       (GObject             *object);

G_DEFINE_DYNAMIC_TYPE (CcAppearancePanel, cc_appearance_panel, CC_TYPE_PANEL)

static void
setup_panel (CcAppearancePanel *panel,
             gboolean           is_active)
{
        static gboolean setup_done = FALSE;

        if (is_active && !setup_done)
        {
                AppearanceData *data;
                GtkWidget *w;
                gchar *uifile;
                GtkBuilder *ui;
                GError *err = NULL;
                gchar *objects[] = {"render_details", "main_notebook", "wp_style_liststore",
                        "wp_color_liststore", "toolbar_style_liststore", NULL};

                /* set up the data */
                uifile = g_build_filename (GNOMECC_GTKBUILDER_DIR, "appearance.ui",
                                NULL);
                ui = gtk_builder_new ();
                gtk_builder_add_objects_from_file (ui, uifile, objects, &err);
                g_free (uifile);

                if (err)
                {
                        g_warning (_("Could not load user interface file: %s"), err->message);
                        g_error_free (err);
                        g_object_unref (ui);
                        return;
                }

                panel->priv->data = data = g_new (AppearanceData, 1);
                data->client = gconf_client_get_default ();
                data->ui = ui;
                data->thumb_factory = gnome_desktop_thumbnail_factory_new (GNOME_DESKTOP_THUMBNAIL_SIZE_NORMAL);

                /* init tabs */
                desktop_init (data, NULL);
                font_init (data);

                /* prepare the main window */
                w = appearance_capplet_get_widget (data, "main_notebook");
                gtk_widget_show_all (w);
                gtk_container_add (GTK_CONTAINER (panel), w);

                w = appearance_capplet_get_widget (data, "theme_vbox");
                gtk_widget_destroy (w);

                setup_done = TRUE;
        }

        if (setup_done && !is_active)
        {
                gnome_wp_xml_save_list (panel->priv->data);
        }
}

static GObject *
cc_appearance_panel_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        CcAppearancePanel      *appearance_panel;

        appearance_panel = CC_APPEARANCE_PANEL (G_OBJECT_CLASS (cc_appearance_panel_parent_class)->constructor (type,
                                                                                                                n_construct_properties,
                                                                                                                construct_properties));

        g_object_set (appearance_panel,
                      "display-name", _("Appearance"),
                      "id", "gnome-appearance-properties.desktop",
                      NULL);

        return G_OBJECT (appearance_panel);
}

static void
cc_appearance_panel_class_init (CcAppearancePanelClass *klass)
{
        GObjectClass  *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = cc_appearance_panel_constructor;
        object_class->finalize = cc_appearance_panel_finalize;

        g_type_class_add_private (klass, sizeof (CcAppearancePanelPrivate));
}

static void
cc_appearance_panel_class_finalize (CcAppearancePanelClass *klass)
{
}

static void
cc_appearance_panel_init (CcAppearancePanel *panel)
{
        panel->priv = CC_APPEARANCE_PANEL_GET_PRIVATE (panel);

        g_signal_connect (panel, "active-changed", G_CALLBACK (setup_panel),
                          NULL);
}

static void
cc_appearance_panel_finalize (GObject *object)
{
        CcAppearancePanel *appearance_panel;

        g_return_if_fail (object != NULL);
        g_return_if_fail (CC_IS_APPEARANCE_PANEL (object));

        appearance_panel = CC_APPEARANCE_PANEL (object);

        g_return_if_fail (appearance_panel->priv != NULL);

        G_OBJECT_CLASS (cc_appearance_panel_parent_class)->finalize (object);
}

void
cc_appearance_panel_register (GIOModule *module)
{
        cc_appearance_panel_register_type (G_TYPE_MODULE (module));
        g_io_extension_point_implement (CC_PANEL_EXTENSION_POINT_NAME,
                                        CC_TYPE_APPEARANCE_PANEL,
                                        "appearance",
                                        10);
}
