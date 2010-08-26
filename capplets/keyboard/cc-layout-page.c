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
#include <gdk/gdkx.h>
#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include <gconf/gconf-client.h>

#include "gconf-property-editor.h"

#include "cc-layout-page.h"
#include "gnome-keyboard-properties-xkb.h"

#define CC_LAYOUT_PAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_LAYOUT_PAGE, CcLayoutPagePrivate))

#define WID(s) GTK_WIDGET (gtk_builder_get_object (builder, s))

struct CcLayoutPagePrivate
{
        gpointer dummy;
};

enum {
        PROP_0,
};

static void     cc_layout_page_class_init     (CcLayoutPageClass *klass);
static void     cc_layout_page_init           (CcLayoutPage      *layout_page);
static void     cc_layout_page_finalize       (GObject             *object);

G_DEFINE_TYPE (CcLayoutPage, cc_layout_page, CC_TYPE_PAGE)

static void
cc_layout_page_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
cc_layout_page_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}


static void
setup_page (CcLayoutPage *page)
{
        GtkBuilder      *builder;
        GError          *error;
        GConfChangeSet  *changeset;
        gchar           *objects[] = {"layouts-vbox", NULL};
        changeset = NULL;

        builder = gtk_builder_new ();

        error = NULL;
        gtk_builder_add_objects_from_file (builder,
                                           GNOMECC_UI_DIR
                                           "/gnome-keyboard-properties-dialog.ui",
                                           objects,
                                           &error);
        if (error != NULL) {
                g_error (_("Could not load user interface file: %s"),
                         error->message);
                g_error_free (error);
                return;
        }

        gtk_widget_show_all (WID ("layouts-vbox"));

        gtk_widget_hide (WID ("xkb_layout_options"));
        gtk_widget_hide (WID ("xkb_layouts_print"));
        gtk_widget_hide (WID ("chk_separate_group_per_window"));
        gtk_widget_hide (WID ("chk_new_windows_inherit_layout"));

        setup_xkb_tabs (builder, NULL);

        gtk_container_add (GTK_CONTAINER (page), WID ("layouts-vbox"));
}

static GObject *
cc_layout_page_constructor (GType                  type,
                            guint                  n_construct_properties,
                            GObjectConstructParam *construct_properties)
{
        CcLayoutPage      *layout_page;

        layout_page = CC_LAYOUT_PAGE (G_OBJECT_CLASS (cc_layout_page_parent_class)->constructor (type,
                                                                                                                n_construct_properties,
                                                                                                                construct_properties));

        g_object_set (layout_page,
                      "display-name", _("Layouts"),
                      "id", "layout",
                      NULL);

        setup_page (layout_page);

        return G_OBJECT (layout_page);
}

static void
cc_layout_page_class_init (CcLayoutPageClass *klass)
{
        GObjectClass  *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = cc_layout_page_get_property;
        object_class->set_property = cc_layout_page_set_property;
        object_class->constructor = cc_layout_page_constructor;
        object_class->finalize = cc_layout_page_finalize;

        g_type_class_add_private (klass, sizeof (CcLayoutPagePrivate));
}

static void
cc_layout_page_init (CcLayoutPage *page)
{
        page->priv = CC_LAYOUT_PAGE_GET_PRIVATE (page);
}

static void
cc_layout_page_finalize (GObject *object)
{
        CcLayoutPage *page;

        g_return_if_fail (object != NULL);
        g_return_if_fail (CC_IS_LAYOUT_PAGE (object));

        page = CC_LAYOUT_PAGE (object);

        g_return_if_fail (page->priv != NULL);


        G_OBJECT_CLASS (cc_layout_page_parent_class)->finalize (object);
}

CcPage *
cc_layout_page_new (void)
{
        GObject *object;

        object = g_object_new (CC_TYPE_LAYOUT_PAGE, NULL);

        return CC_PAGE (object);
}
