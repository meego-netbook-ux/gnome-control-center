/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Intel Corporation.
 *
 * Written by: Jussi Kukkonen <jku@linux.intel.com>
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

#include "cc-power-panel.h"

/* largest numeric value shown */
#define SLEEP_MAX 45 * 60
#define IDLE_MAX 45 * 60

#define SLEEP_NEVER SLEEP_MAX + 31
#define IDLE_NEVER IDLE_MAX + 31


#define GPM_DIR "/apps/gnome-power-manager"
#define GSS_DIR "/apps/gnome-screensaver"

/* values are "hibernate", "suspend" or "nothing" */
#define SLEEP_TYPE_BATTERY_KEY GPM_DIR"/actions/sleep_type_battery"
#define SLEEP_TYPE_AC_KEY GPM_DIR"/actions/sleep_type_ac"
#define SLEEP_TYPE "suspend"

/* seconds until going to sleep */
#define SLEEP_AC_KEY GPM_DIR"/timeout/sleep_computer_ac"
#define SLEEP_BATTERY_KEY GPM_DIR"/timeout/sleep_computer_battery"

/* minutes until idle */
#define IDLE_KEY GSS_DIR"/idle_delay"

#define CC_POWER_PANEL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_POWER_PANEL, CcPowerPanelPrivate))

#define WID(s) GTK_WIDGET (gtk_builder_get_object (builder, s))

struct CcPowerPanelPrivate
{
        GtkWidget *idle_scale;
        guint idle_id;

        GtkWidget *sleep_scale;
        guint sleep_id;

        GConfClient *gconf;
};

G_DEFINE_DYNAMIC_TYPE (CcPowerPanel, cc_power_panel, CC_TYPE_PANEL)

static char*
scale_format_value (GtkScale *scale, gdouble value, CcPowerPanel *panel)
{
        int secs = (int) value;
        int max;

        if (scale == GTK_SCALE (panel->priv->idle_scale)) {
                max = IDLE_NEVER;
        } else {
                max = SLEEP_NEVER;
        }

        if (secs < max) {
                /* TRANSLATORS: texts below idle/sleep scales */
                return g_strdup_printf (ngettext ("%d minute",
                                                  "%d minutes",
                                                  (secs+30)/60),
                                        (secs+30)/60);
        } else {
                return g_strdup (_("Never"));
        }
}

static void
sleep_scale_value_changed (GtkRange *range, CcPowerPanel *panel)
{
        GConfChangeSet *set;
        int secs;
        
        secs = (int)gtk_range_get_value (range);
        set = gconf_change_set_new ();


        if (secs < SLEEP_NEVER) {
                gconf_change_set_set_string (set, 
                                             SLEEP_TYPE_BATTERY_KEY,
                                             SLEEP_TYPE);
                gconf_change_set_set_string (set, 
                                             SLEEP_TYPE_AC_KEY,
                                             SLEEP_TYPE);
                gconf_change_set_set_int (set, 
                                          SLEEP_BATTERY_KEY,
                                          secs);
                gconf_change_set_set_int (set, 
                                          SLEEP_AC_KEY,
                                          secs);
        } else {
                gconf_change_set_set_string (set, 
                                             SLEEP_TYPE_BATTERY_KEY,
                                             "nothing");
                gconf_change_set_set_string (set, 
                                             SLEEP_TYPE_AC_KEY,
                                             "nothing");
        }

        gconf_client_commit_change_set (panel->priv->gconf,
                                        set, TRUE, NULL);
        gconf_change_set_unref (set);
}

static void
idle_scale_value_changed (GtkRange *range, CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        int secs;

        secs = (int)gtk_range_get_value (range);
        if (secs < IDLE_NEVER) {
                gconf_client_set_int (priv->gconf, IDLE_KEY, (secs+30)/60, NULL);
        } else {
                gconf_client_set_int (priv->gconf, IDLE_KEY, G_MAXINT, NULL);
        }
}

static void
update_sleep_toggle (CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        int sleep = SLEEP_NEVER;
        char *sleep_type = "nothing";
        GError *error = NULL;

        sleep_type = gconf_client_get_string (priv->gconf, SLEEP_TYPE_BATTERY_KEY,
                                              &error);
        if (error) {
                g_warning ("Could not read key %s: %s",
                           SLEEP_TYPE_BATTERY_KEY, error->message);
                g_error_free (error);
                error = NULL;
        }

        if (!sleep_type || strcmp (sleep_type, "nothing") == 0) {
                sleep = SLEEP_NEVER;
        } else {
                sleep = gconf_client_get_int (priv->gconf, SLEEP_BATTERY_KEY,
                                              &error);
                if (error) {
                        g_warning ("Could not read key %s: %s",
                                   SLEEP_BATTERY_KEY, error->message);
                        g_error_free (error);
                        error = NULL;
                }
                g_free (sleep_type);
        }

        sleep = CLAMP (sleep, 60, SLEEP_NEVER);

        g_signal_handler_block (priv->sleep_scale, priv->sleep_id);
        gtk_range_set_value (GTK_RANGE (priv->sleep_scale), sleep);
        g_signal_handler_unblock (priv->sleep_scale, priv->sleep_id);
}

static void
update_idle_toggle (CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        int idle_secs, current_secs, idle;
        GError *error = NULL;

        idle = gconf_client_get_int (priv->gconf, IDLE_KEY, &error);
        if (error) {
                g_warning ("Could not read key %s: %s",
                           IDLE_KEY, error->message);
                g_error_free (error);
                error = NULL;
                idle_secs = IDLE_NEVER;
        } else {
                idle_secs = 60 * CLAMP (idle, 1, (IDLE_NEVER + 30)/60);
        }

        /* no need to move the slider if we're on the right minute */
        current_secs = (int)gtk_range_get_value (
                GTK_RANGE (panel->priv->idle_scale));
        if (idle != (current_secs+30)/60) {
                g_signal_handler_block (priv->idle_scale, priv->idle_id);
                gtk_range_set_value (GTK_RANGE (priv->idle_scale), idle_secs);
                g_signal_handler_unblock (priv->idle_scale, priv->idle_id);
        }
}

static void
gconf_notify_gpm (GConfClient *gconf, guint id,
                  GConfEntry *entry, CcPowerPanel *panel)
{
        update_sleep_toggle (panel);
}

static void
gconf_notify_gss (GConfClient *gconf, guint id,
                  GConfEntry *entry, CcPowerPanel *panel)
{
        update_idle_toggle (panel);
}

static void
setup_panel (CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        GError *error = NULL;
        GtkBuilder *builder;
        GtkWidget *widget;

        builder = gtk_builder_new ();

        gtk_builder_add_from_file (builder,
                                   GNOMECC_UI_DIR "/power.ui",
                                   &error);

        if (error != NULL) {
                g_error (_("Could not load user interface file: %s"), error->message);
                g_error_free (error);
                return;
        }

        priv->gconf = gconf_client_get_default ();
        gconf_client_add_dir (priv->gconf,
                              GPM_DIR,
                              GCONF_CLIENT_PRELOAD_NONE,
                              NULL);
        gconf_client_add_dir (priv->gconf,
                              GSS_DIR,
                              GCONF_CLIENT_PRELOAD_NONE,
                              NULL);

        gconf_client_notify_add (priv->gconf, SLEEP_TYPE_BATTERY_KEY,
                                 (GConfClientNotifyFunc) gconf_notify_gpm,
                                 panel, NULL, NULL);
        gconf_client_notify_add (priv->gconf, SLEEP_BATTERY_KEY,
                                 (GConfClientNotifyFunc) gconf_notify_gpm,
                                 panel, NULL, NULL);
        gconf_client_notify_add (priv->gconf, IDLE_KEY,
                                 (GConfClientNotifyFunc) gconf_notify_gss,
                                 panel, NULL, NULL);

        widget = WID ("main_vbox");
        gtk_widget_reparent (widget, GTK_WIDGET (panel));

        priv->idle_scale = WID ("idle_scale");
        gtk_range_set_range (GTK_RANGE (priv->idle_scale), 60, IDLE_NEVER);
        gtk_range_set_increments (GTK_RANGE (priv->idle_scale), 60, 300);
        g_signal_connect (priv->idle_scale, "format-value",
                          G_CALLBACK (scale_format_value), panel);
        priv->idle_id = g_signal_connect (priv->idle_scale,
                                          "value-changed",
                                          G_CALLBACK (idle_scale_value_changed),
                                          panel);

        priv->sleep_scale = WID ("sleep_scale");
        gtk_range_set_range (GTK_RANGE (priv->sleep_scale), 1, SLEEP_NEVER);
        gtk_range_set_increments (GTK_RANGE (priv->sleep_scale), 60, 300);
        g_signal_connect (priv->sleep_scale, "format-value",
                          G_CALLBACK (scale_format_value), panel);
        priv->sleep_id = g_signal_connect (priv->sleep_scale,
                                           "value-changed",
                                           G_CALLBACK (sleep_scale_value_changed),
                                           panel);

        update_idle_toggle (panel);
        update_sleep_toggle (panel);

        g_object_unref (builder);
}

static GObject *
cc_power_panel_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        CcPowerPanel      *power_panel;

        power_panel = CC_POWER_PANEL (G_OBJECT_CLASS (cc_power_panel_parent_class)->constructor
                                      (type, n_construct_properties, construct_properties));

        g_object_set (power_panel,
                      "display-name", _("Power and brightness"),
                      "id", "power-properties.desktop",
                      NULL);

        setup_panel (power_panel);

        return G_OBJECT (power_panel);
}

static void
cc_power_panel_finalize (GObject *object)
{
        CcPowerPanel *power_panel;

        g_return_if_fail (object != NULL);
        g_return_if_fail (CC_IS_POWER_PANEL (object));

        power_panel = CC_POWER_PANEL (object);

        g_return_if_fail (power_panel->priv != NULL);

        G_OBJECT_CLASS (cc_power_panel_parent_class)->finalize (object);
}

static void
cc_power_panel_dispose (GObject *object)
{
        CcPowerPanel *panel;

        g_return_if_fail (CC_IS_POWER_PANEL (object));

        panel = CC_POWER_PANEL (object);

        if (panel->priv->gconf) {
                g_object_unref (panel->priv->gconf);
                panel->priv->gconf = NULL;
        } 
}

static void
cc_power_panel_class_init (CcPowerPanelClass *klass)
{
        GObjectClass  *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = cc_power_panel_constructor;
        object_class->finalize = cc_power_panel_finalize;
        object_class->dispose = cc_power_panel_dispose;

        g_type_class_add_private (klass, sizeof (CcPowerPanelPrivate));
}

static void
cc_power_panel_class_finalize (CcPowerPanelClass *klass)
{
}

static void
cc_power_panel_init (CcPowerPanel *panel)
{
        panel->priv = CC_POWER_PANEL_GET_PRIVATE (panel);
}

void
cc_power_panel_register (GIOModule *module)
{
        cc_power_panel_register_type (G_TYPE_MODULE (module));
        g_io_extension_point_implement (CC_PANEL_EXTENSION_POINT_NAME,
                                        CC_TYPE_POWER_PANEL,
                                        "power",
                                        10);
}
