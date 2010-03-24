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

#define SLEEP_NEVER SLEEP_MAX + 30
#define IDLE_NEVER IDLE_MAX + 30


#define GPM_DIR "/apps/gnome-power-manager"
#define SESSION_DIR "/desktop/gnome/session"

/* values are "hibernate", "suspend" or "nothing" */
#define SLEEP_TYPE_BATTERY_KEY GPM_DIR"/actions/sleep_type_battery"
#define SLEEP_TYPE_AC_KEY GPM_DIR"/actions/sleep_type_ac"
#define SLEEP_TYPE "suspend"

/* seconds until going to sleep */
#define SLEEP_AC_KEY GPM_DIR"/timeout/sleep_computer_ac"
#define SLEEP_BATTERY_KEY GPM_DIR"/timeout/sleep_computer_battery"

/* minutes until idle */
#define IDLE_KEY SESSION_DIR"/idle_delay"

#define CC_POWER_PANEL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_POWER_PANEL, CcPowerPanelPrivate))

#define WID(s) GTK_WIDGET (gtk_builder_get_object (builder, s))
#define ROUND_MINUTES(sec) ((sec + 30) / 60)

struct CcPowerPanelPrivate
{
        GtkWidget *idle_scale;
        guint idle_id;

        GtkWidget *sleep_scale;
        guint sleep_id;

        GConfClient *gconf;
        int gconf_idle;               /* minutes of inactivity until idle */
        int gconf_sleep;              /* seconds of idle-state until sleep */
        gboolean gconf_sleep_enabled; /* sleep mode != "nothing" */
};

G_DEFINE_DYNAMIC_TYPE (CcPowerPanel, cc_power_panel, CC_TYPE_PANEL)

static char*
scale_format_value (GtkScale *scale, gdouble value, CcPowerPanel *panel)
{
        int secs = (int)value;
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
                                                  ROUND_MINUTES (secs)),
                                        ROUND_MINUTES (secs));
        } else {
                return g_strdup (_("Never"));
        }
}

static void
save_gconf (CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        GConfChangeSet *set;

        set = gconf_change_set_new ();

        gconf_change_set_set_int (set, IDLE_KEY, priv->gconf_idle);
        if (priv->gconf_sleep_enabled) {
                gconf_change_set_set_string (set, 
                                             SLEEP_TYPE_BATTERY_KEY,
                                             SLEEP_TYPE);
                gconf_change_set_set_string (set, 
                                             SLEEP_TYPE_AC_KEY,
                                             SLEEP_TYPE);
                gconf_change_set_set_int (set, 
                                          SLEEP_BATTERY_KEY,
                                          priv->gconf_sleep);
                gconf_change_set_set_int (set, 
                                          SLEEP_AC_KEY,
                                          priv->gconf_sleep);
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
sleep_scale_value_changed (GtkRange *range, CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        int sleep;
        int sleep_mins;

        sleep = (int)gtk_range_get_value (range);
        sleep_mins = ROUND_MINUTES (sleep);

        if (sleep != SLEEP_NEVER &&
            priv->gconf_idle != 0 &&
            sleep_mins <= priv->gconf_idle) {
                /* need to move idle time to make space for sleep */
                if (sleep_mins <= 1) {
                        priv->gconf_idle = 0;
                } else {
                        priv->gconf_idle = CLAMP (sleep_mins - 1,
                                                  1, IDLE_MAX);
                }
        }

        if (sleep == SLEEP_NEVER) {
                priv->gconf_sleep_enabled = FALSE;
        } else {
                priv->gconf_sleep_enabled = TRUE;
                priv->gconf_sleep = sleep - priv->gconf_idle * 60;
        }

        save_gconf (panel);
}

static void
idle_scale_value_changed (GtkRange *range, CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        int idle, diff, idle_mins;

        idle = (int)gtk_range_get_value (range);
        idle_mins = ROUND_MINUTES (idle);

        if (idle == IDLE_NEVER) {
                diff = - 60 * priv->gconf_idle;
                priv->gconf_idle = 0;
        } else {
                diff = 60 * (idle_mins - priv->gconf_idle);
                priv->gconf_idle = idle_mins;
        }

        if (priv->gconf_sleep_enabled) {
                priv->gconf_sleep = CLAMP (priv->gconf_sleep - diff,
                                           60, SLEEP_NEVER - 1);
        }

        save_gconf (panel);
}

static void
get_sleep_from_gconf (CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        char *sleep_type;
        GError *error = NULL;

        sleep_type = gconf_client_get_string (priv->gconf,
                                              SLEEP_TYPE_BATTERY_KEY,
                                              &error);
        if (error) {
                g_warning ("Could not read key %s: %s",
                           SLEEP_TYPE_BATTERY_KEY, error->message);
                g_error_free (error);
                error = NULL;
        }

        if (!sleep_type || strcmp (sleep_type, "nothing") == 0) {
                priv->gconf_sleep_enabled = FALSE;
        } else {
                priv->gconf_sleep_enabled = TRUE;
                priv->gconf_sleep = gconf_client_get_int (priv->gconf,
                                                          SLEEP_BATTERY_KEY,
                                                          &error);
                if (error) {
                        g_warning ("Could not read key %s: %s",
                                   SLEEP_BATTERY_KEY, error->message);
                        g_error_free (error);
                        error = NULL;
                        priv->gconf_sleep_enabled = FALSE;
                }
        }
        g_free (sleep_type);
}

static void
get_idle_from_gconf (CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        GError *error = NULL;

        priv->gconf_idle = gconf_client_get_int (priv->gconf,
                                                 IDLE_KEY,
                                                 &error);
        if (error) {
                g_warning ("Could not read key %s: %s",
                           IDLE_KEY, error->message);
                g_error_free (error);
                error = NULL;
                priv->gconf_idle = 0;
        }
}

static void
update_sleep_range (CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        int sleep;

        if (priv->gconf_sleep_enabled) {
                sleep = CLAMP (priv->gconf_sleep + priv->gconf_idle * 60,
                               60, SLEEP_NEVER - 1);
        } else {
                sleep = SLEEP_NEVER;
        }

        gtk_range_set_value (GTK_RANGE (priv->sleep_scale), sleep);
}

static void
update_idle_range (CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        int idle, idle_mins;

        /* no need to move the slider if we're on the right minute */
        idle = (int)gtk_range_get_value (GTK_RANGE (priv->idle_scale));
        if (idle == IDLE_NEVER) {
                idle_mins = 0;
        } else {
                idle_mins = ROUND_MINUTES (idle);
        }

        if (priv->gconf_idle != idle_mins) {
                if (priv->gconf_idle == 0) {
                        idle = IDLE_NEVER;
                } else {
                        idle = CLAMP (priv->gconf_idle * 60,
                                      60, IDLE_MAX);
                }
                gtk_range_set_value (GTK_RANGE (priv->idle_scale), idle);
        }
}

static void
gconf_notify_gpm (GConfClient *gconf, guint id,
                  GConfEntry *entry, CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);

        get_sleep_from_gconf (panel);

        g_signal_handler_block (priv->sleep_scale, priv->sleep_id);
        update_sleep_range (panel);
        g_signal_handler_unblock (priv->sleep_scale, priv->sleep_id);
}

static void
gconf_notify_session (GConfClient *gconf, guint id,
                      GConfEntry *entry, CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);

        get_idle_from_gconf (panel);

        g_signal_handler_block (priv->idle_scale, priv->idle_id);
        update_idle_range (panel);
        g_signal_handler_unblock (priv->idle_scale, priv->idle_id);
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
                              SESSION_DIR,
                              GCONF_CLIENT_PRELOAD_NONE,
                              NULL);

        gconf_client_notify_add (priv->gconf, SLEEP_TYPE_BATTERY_KEY,
                                 (GConfClientNotifyFunc) gconf_notify_gpm,
                                 panel, NULL, NULL);
        gconf_client_notify_add (priv->gconf, SLEEP_BATTERY_KEY,
                                 (GConfClientNotifyFunc) gconf_notify_gpm,
                                 panel, NULL, NULL);
        gconf_client_notify_add (priv->gconf, IDLE_KEY,
                                 (GConfClientNotifyFunc) gconf_notify_session,
                                 panel, NULL, NULL);

        widget = WID ("main_vbox");
        gtk_widget_reparent (widget, GTK_WIDGET (panel));

        priv->idle_scale = WID ("idle_scale");
        gtk_range_set_range (GTK_RANGE (priv->idle_scale), 60, IDLE_NEVER);
        gtk_range_set_increments (GTK_RANGE (priv->idle_scale), 60, 300);
        g_signal_connect (priv->idle_scale, "format-value",
                          G_CALLBACK (scale_format_value), panel);
        get_idle_from_gconf (panel);
        update_idle_range (panel);
        priv->idle_id = g_signal_connect (priv->idle_scale,
                                          "value-changed",
                                          G_CALLBACK (idle_scale_value_changed),
                                          panel);

        priv->sleep_scale = WID ("sleep_scale");
        gtk_range_set_range (GTK_RANGE (priv->sleep_scale), 60, SLEEP_NEVER);
        gtk_range_set_increments (GTK_RANGE (priv->sleep_scale), 60, 300);
        g_signal_connect (priv->sleep_scale, "format-value",
                          G_CALLBACK (scale_format_value), panel);
        get_sleep_from_gconf (panel);
        update_sleep_range (panel);
        priv->sleep_id = g_signal_connect (priv->sleep_scale,
                                           "value-changed",
                                           G_CALLBACK (sleep_scale_value_changed),
                                           panel);

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
