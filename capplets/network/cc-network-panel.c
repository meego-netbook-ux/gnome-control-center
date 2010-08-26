/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Red Hat, Inc.
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

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include <gconf/gconf-client.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnomeui/gnome-desktop-thumbnail.h>

#include "cc-network-panel.h"

#define CC_NETWORK_PANEL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_NETWORK_PANEL, CcNetworkPanelPrivate))

#define GNOMECC_GNP_UI_FILE (GNOMECC_UI_DIR "/gnome-network-properties.ui")

struct CcNetworkPanelPrivate
{
        GConfClient *client;
        GtkBuilder *builder;
};

enum {
        PROP_0,
};

static void     cc_network_panel_class_init     (CcNetworkPanelClass *klass);
static void     cc_network_panel_init           (CcNetworkPanel      *network_panel);
static void     cc_network_panel_dispose        (GObject             *object);

G_DEFINE_DYNAMIC_TYPE (CcNetworkPanel, cc_network_panel, CC_TYPE_PANEL)

static GtkWidget*
_gtk_builder_get_widget (GtkBuilder *builder, const gchar *name)
{
        return GTK_WIDGET (gtk_builder_get_object (builder, name));
}


static void
setup_panel (CcNetworkPanel *panel)
{
        GtkBuilder  *builder;
        GError *error = NULL;
        gchar *builder_widgets[] = {"network_proxy_vbox", "adjustment1",
                                    "adjustment2", "adjustment3", "adjustment4",
                                    "delete_button_img", NULL};
        GConfClient *client;
        GtkWidget   *widget;

        client = gconf_client_get_default ();
        gconf_client_add_dir (client, "/system/http_proxy",
                              GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
        gconf_client_add_dir (client, "/system/proxy",
                              GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

        builder = gtk_builder_new ();
        if (gtk_builder_add_objects_from_file (builder, GNOMECC_GNP_UI_FILE,
                                               builder_widgets, &error) == 0) {
                g_warning ("Could not load main dialog: %s",
                           error->message);
                g_error_free (error);
                g_object_unref (builder);
                g_object_unref (client);
                return;
        }

        setup_dialog (builder);
        widget = _gtk_builder_get_widget (builder, "network_proxy_vbox");
        gtk_widget_show_all (widget);
        gtk_container_add (GTK_CONTAINER (panel), widget);
}

static GObject *
cc_network_panel_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        CcNetworkPanel      *network_panel;

        network_panel = CC_NETWORK_PANEL (G_OBJECT_CLASS (cc_network_panel_parent_class)->constructor (type,
                                                                                                                n_construct_properties,
                                                                                                                construct_properties));

        g_object_set (network_panel,
                      "display-name", _("Network"),
                      "id", "gnome-network-properties.desktop",
                      NULL);

        setup_panel (network_panel);

        return G_OBJECT (network_panel);
}

static void
cc_network_panel_class_init (CcNetworkPanelClass *klass)
{
        GObjectClass  *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = cc_network_panel_constructor;
        object_class->finalize = cc_network_panel_dispose;

        g_type_class_add_private (klass, sizeof (CcNetworkPanelPrivate));
}

static void
cc_network_panel_class_finalize (CcNetworkPanelClass *klass)
{
}

static void
cc_network_panel_init (CcNetworkPanel *panel)
{
        panel->priv = CC_NETWORK_PANEL_GET_PRIVATE (panel);
}

static void
cc_network_panel_dispose (GObject *object)
{
        CcNetworkPanelPrivate *priv = CC_NETWORK_PANEL (object)->priv;

        if (priv->builder)
        {
                g_object_unref (priv->builder);
                priv->builder = NULL;
        }

        if (priv->client)
        {
                g_object_unref (priv->client);
                priv->client = NULL;
        }

        G_OBJECT_CLASS (cc_network_panel_parent_class)->dispose (object);
}

void
cc_network_panel_register (GIOModule *module)
{
        cc_network_panel_register_type (G_TYPE_MODULE (module));
        g_io_extension_point_implement (CC_PANEL_EXTENSION_POINT_NAME,
                                        CC_TYPE_NETWORK_PANEL,
                                        "network",
                                        10);
}
