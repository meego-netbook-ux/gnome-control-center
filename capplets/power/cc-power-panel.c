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
#define MEEGO_DIR "/desktop/meego"

/* values are "hibernate", "suspend" or "nothing" */
#define GPM_SLEEP_TYPE_BATTERY_KEY GPM_DIR"/actions/sleep_type_battery"
#define GPM_SLEEP_TYPE_AC_KEY GPM_DIR"/actions/sleep_type_ac"
#define GPM_SLEEP_TYPE "suspend"

/* seconds until going to sleep */
#define GPM_SLEEP_AC_KEY GPM_DIR"/timeout/sleep_computer_ac"
#define GPM_SLEEP_BATTERY_KEY GPM_DIR"/timeout/sleep_computer_battery"

/* sleep for meego, seconds until going to sleep */
#define MEEGO_SLEEP_KEY MEEGO_DIR"/panel-devices/timeout/sleep_computer"

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

        int clamp;

        GConfClient *gconf;
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
        int idle, idle_mins, sleep;
        gboolean sleep_enabled;

        idle = (int)gtk_range_get_value (GTK_RANGE (priv->idle_scale));
        sleep = (int)gtk_range_get_value (GTK_RANGE (priv->sleep_scale));

        if (idle == IDLE_NEVER) {
                idle = 0;
        }
        /* round values to nearest minute: idle gconf key is in minutes
         * and sleep looks visually better if it's "in sync" with idle */
        idle_mins = ROUND_MINUTES (idle);

        if (sleep == SLEEP_NEVER) {
                sleep = -1;
                sleep_enabled = FALSE;
        } else {
                sleep = MAX (0, sleep - idle);
                sleep = ROUND_MINUTES (sleep) * 60;
                sleep_enabled = TRUE;
        }

        set = gconf_change_set_new ();

        gconf_change_set_set_int (set, IDLE_KEY, idle_mins);

#ifdef HAVE_MOBLIN
        gconf_change_set_set_int (set,
                                  MEEGO_SLEEP_KEY,
                                  sleep);
#else
        if (sleep_enabled) {
                gconf_change_set_set_string (set,
                                             GPM_SLEEP_TYPE_BATTERY_KEY,
                                             GPM_SLEEP_TYPE);
                gconf_change_set_set_string (set,
                                             GPM_SLEEP_TYPE_AC_KEY,
                                             GPM_SLEEP_TYPE);
                gconf_change_set_set_int (set,
                                          GPM_SLEEP_BATTERY_KEY,
                                          sleep);
                gconf_change_set_set_int (set,
                                          GPM_SLEEP_AC_KEY,
                                          sleep);
        } else {
                gconf_change_set_set_string (set,
                                             GPM_SLEEP_TYPE_BATTERY_KEY,
                                             "nothing");
                gconf_change_set_set_string (set,
                                             GPM_SLEEP_TYPE_AC_KEY,
                                             "nothing");
        }
#endif

        gconf_client_commit_change_set (panel->priv->gconf,
                                        set, TRUE, NULL);
        gconf_change_set_unref (set);
}

static void
idle_scale_value_changed (GtkRange *range, CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        int idle, sleep;

        priv->clamp = -1;
        idle = (int)gtk_range_get_value (GTK_RANGE (priv->idle_scale));
        sleep = (int)gtk_range_get_value (GTK_RANGE (priv->sleep_scale));

        if (idle > sleep) {
                /* this can happen e.g. when clicking on through */
                g_signal_handler_block (GTK_RANGE (priv->sleep_scale), priv->sleep_id);
                gtk_range_set_value (GTK_RANGE (priv->sleep_scale), idle);
                g_signal_handler_unblock (GTK_RANGE (priv->sleep_scale), priv->sleep_id);
        }

        save_gconf (panel);
}

static void
sleep_scale_value_changed (GtkRange *range, CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        int idle, sleep;

        priv->clamp = -1;
        idle = (int)gtk_range_get_value (GTK_RANGE (priv->idle_scale));
        sleep = (int)gtk_range_get_value (GTK_RANGE (priv->sleep_scale));

        if (idle > sleep) {
                /* this can happen e.g. when clicking on through */
                g_signal_handler_block (GTK_RANGE (priv->idle_scale), priv->idle_id);
                gtk_range_set_value (GTK_RANGE (priv->idle_scale), sleep);
                g_signal_handler_unblock (GTK_RANGE (priv->idle_scale), priv->idle_id);

        }
        save_gconf (panel);
}


static gboolean
idle_range_scrolled (GtkRange      *range,
                     GtkScrollType  scroll,
                     gdouble        value,
                     CcPowerPanel  *panel)
{
        int sleep;
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        GtkRange *sleep_range = GTK_RANGE (priv->sleep_scale);

        if (priv->clamp < 0) {
                priv->clamp = (int)gtk_range_get_value (sleep_range);
        }

        sleep = MAX (priv->clamp, (int)gtk_range_get_value (range));

        g_signal_handler_block (sleep_range, priv->sleep_id);
        gtk_range_set_value (sleep_range, sleep);
        g_signal_handler_unblock (sleep_range, priv->sleep_id);

        return FALSE;
}

static gboolean
sleep_range_scrolled (GtkRange      *range,
                      GtkScrollType  scroll,
                      gdouble        value,
                      CcPowerPanel  *panel)
{
        int idle;
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        GtkRange *idle_range = GTK_RANGE (priv->idle_scale);

        if (priv->clamp < 0) {
                priv->clamp = (int)gtk_range_get_value (idle_range);
        }

        idle = MIN (priv->clamp, (int)gtk_range_get_value (range));

        g_signal_handler_block (idle_range, priv->idle_id);
        gtk_range_set_value (idle_range, idle);
        g_signal_handler_unblock (idle_range, priv->idle_id);

        return FALSE;
}


static void
update_ranges_from_gconf (CcPowerPanel *panel)
{
        CcPowerPanelPrivate *priv = CC_POWER_PANEL_GET_PRIVATE (panel);
        int sleep = SLEEP_NEVER;
        int idle = IDLE_NEVER;
        GError *error = NULL;

        idle = gconf_client_get_int (priv->gconf,
                                     IDLE_KEY,
                                     &error);
        if (error) {
                g_warning ("Could not read key %s: %s",
                           IDLE_KEY, error->message);
                g_error_free (error);
                error = NULL;
        }

        if (idle == 0) {
                idle = IDLE_NEVER;
                sleep = SLEEP_NEVER;
        } else {
                idle = CLAMP (60 * idle, 60, IDLE_MAX);

#ifdef HAVE_MOBLIN
                sleep = gconf_client_get_int (priv->gconf,
                                              MEEGO_SLEEP_KEY,
                                              &error);
                if (error) {
                        g_warning ("Could not read key %s: %s",
                                   MEEGO_SLEEP_KEY, error->message);
                        g_error_free (error);
                        error = NULL;
                }

                if (sleep < 0) {
                        sleep = SLEEP_NEVER;
                } else {
                        sleep = CLAMP (sleep + idle, 60, SLEEP_MAX);
                }
#else
{
                char *sleep_type;
                sleep_type = gconf_client_get_string (priv->gconf,
                                                      GPM_SLEEP_TYPE_BATTERY_KEY,
                                                      &error);
                if (error) {
                        g_warning ("Could not read key %s: %s",
                                   GPM_SLEEP_TYPE_BATTERY_KEY, error->message);
                        g_error_free (error);
                        error = NULL;
                }

                if (!sleep_type || strcmp (sleep_type, "nothing") == 0) {
                        sleep = SLEEP_NEVER;
                } else {
                        sleep = gconf_client_get_int (priv->gconf,
                                                      GPM_SLEEP_BATTERY_KEY,
                                                      &error);
                        if (error) {
                                g_warning ("Could not read key %s: %s",
                                           GPM_SLEEP_BATTERY_KEY, error->message);
                                g_error_free (error);
                                error = NULL;
                                sleep = SLEEP_NEVER;
                        } else {
                                sleep = CLAMP (sleep + idle, 60, SLEEP_MAX);
                        }
                }
                g_free (sleep_type);
}
#endif
        }

        g_signal_handler_block (priv->idle_scale, priv->idle_id);
        gtk_range_set_value (GTK_RANGE (priv->idle_scale), idle);
        g_signal_handler_unblock (priv->idle_scale, priv->idle_id);

        g_signal_handler_block (priv->sleep_scale, priv->sleep_id);
        gtk_range_set_value (GTK_RANGE (priv->sleep_scale), sleep);
        g_signal_handler_unblock (priv->sleep_scale, priv->sleep_id);
}

static gboolean
scale_focus_out (GtkWidget     *widget,
                 GdkEventFocus *event,
                 CcPowerPanel  *panel)
{
        /* there are cases when change-value gets fired but not value-changed 
         * follows. This is here to re-init in that case */
        panel->priv->clamp = -1;
        return FALSE;
}

static void
gconf_notify (GConfClient *gconf, guint id,
              GConfEntry *entry, CcPowerPanel *panel)
{
        update_ranges_from_gconf (panel);
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
#ifdef HAVE_MOBLIN
        gconf_client_add_dir (priv->gconf,
                              MEEGO_DIR,
                              GCONF_CLIENT_PRELOAD_NONE,
                              NULL);
        gconf_client_notify_add (priv->gconf, MEEGO_SLEEP_KEY,
                                 (GConfClientNotifyFunc) gconf_notify,
                                 panel, NULL, NULL);
#else
        gconf_client_add_dir (priv->gconf,
                              GPM_DIR,
                              GCONF_CLIENT_PRELOAD_NONE,
                              NULL);
        gconf_client_notify_add (priv->gconf, GPM_SLEEP_TYPE_BATTERY_KEY,
                                 (GConfClientNotifyFunc) gconf_notify,
                                 panel, NULL, NULL);
        gconf_client_notify_add (priv->gconf, GPM_SLEEP_BATTERY_KEY,
                                 (GConfClientNotifyFunc) gconf_notify,
                                 panel, NULL, NULL);
#endif
        gconf_client_add_dir (priv->gconf,
                              SESSION_DIR,
                              GCONF_CLIENT_PRELOAD_NONE,
                              NULL);
        gconf_client_notify_add (priv->gconf, IDLE_KEY,
                                 (GConfClientNotifyFunc) gconf_notify,
                                 panel, NULL, NULL);

        widget = WID ("main_vbox");
        gtk_widget_reparent (widget, GTK_WIDGET (panel));

        priv->idle_scale = WID ("idle_scale");
        gtk_range_set_range (GTK_RANGE (priv->idle_scale), 60, IDLE_NEVER);
        gtk_range_set_increments (GTK_RANGE (priv->idle_scale), 60, 300);
        g_signal_connect (priv->idle_scale, "format-value",
                          G_CALLBACK (scale_format_value), panel);
        g_signal_connect (priv->idle_scale, "change-value",
                          G_CALLBACK (idle_range_scrolled), panel);
        g_signal_connect (priv->idle_scale, "focus-out-event",
                          G_CALLBACK (scale_focus_out), panel);
        priv->idle_id = g_signal_connect (priv->idle_scale,
                                          "value-changed",
                                          G_CALLBACK (idle_scale_value_changed),
                                          panel);

        priv->sleep_scale = WID ("sleep_scale");
        gtk_range_set_range (GTK_RANGE (priv->sleep_scale), 60, SLEEP_NEVER);
        gtk_range_set_increments (GTK_RANGE (priv->sleep_scale), 60, 300);
        g_signal_connect (priv->sleep_scale, "format-value",
                          G_CALLBACK (scale_format_value), panel);
        g_signal_connect (priv->sleep_scale, "change-value",
                          G_CALLBACK (sleep_range_scrolled), panel);
        g_signal_connect (priv->sleep_scale, "focus-out-event",
                          G_CALLBACK (scale_focus_out), panel);
        priv->sleep_id = g_signal_connect (priv->sleep_scale,
                                           "value-changed",
                                           G_CALLBACK (sleep_scale_value_changed),
                                           panel);

        update_ranges_from_gconf (panel);

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
        panel->priv->clamp = -1;
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
