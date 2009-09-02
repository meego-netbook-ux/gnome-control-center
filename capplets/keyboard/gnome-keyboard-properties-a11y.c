/* -*- mode: c; style: linux -*- */

/*
 * Copyright (C) 2007 The GNOME Foundation
 * Written by Denis Washington <denisw@svn.gnome.org>
 * All Rights Reserved
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gnome-keyboard-properties-a11y.h"
#include <gconf/gconf-client.h>
#include "gconf-property-editor.h"
#include "capplet-util.h"

#define CONFIG_ROOT "/desktop/gnome/accessibility/keyboard"
#define NWID(s) glade_xml_get_widget (notifications_dialog, s)

#if HAVE_MOBLIN
#define GLADE_FILE  "/gnome-keyboard-properties-moblin.glade"
#else
#define GLADE_FILE  "/gnome-keyboard-properties.glade"
#endif

static GladeXML *notifications_dialog = NULL;

static void
stickykeys_enable_toggled_cb (GtkWidget *w, GladeXML *dialog)
{
	gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	gtk_widget_set_sensitive (WID ("stickykeys_two_key_off"), active);
	if (notifications_dialog)
		gtk_widget_set_sensitive (NWID ("stickykeys_notifications_box"), active);
}

static void
slowkeys_enable_toggled_cb (GtkWidget *w, GladeXML *dialog)
{
	gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	gtk_widget_set_sensitive (WID ("slowkeys_delay_box"), active);
	if (notifications_dialog)
		gtk_widget_set_sensitive (NWID ("slowkeys_notifications_box"), active);
}

static void
bouncekeys_enable_toggled_cb (GtkWidget *w, GladeXML *dialog)
{
	gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	gtk_widget_set_sensitive (WID ("bouncekeys_delay_box"), active);
	if (notifications_dialog)
		gtk_widget_set_sensitive (NWID ("bouncekeys_notifications_box"), active);
}

static void
visual_bell_enable_toggled_cb (GtkWidget *w, GladeXML *dialog)
{
	gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	if (notifications_dialog) {
		gtk_widget_set_sensitive (NWID ("visual_bell_titlebar"), active);
		gtk_widget_set_sensitive (NWID ("visual_bell_fullscreen"), active);
	}
}

static GConfEnumStringPair bell_flash_enums[] = {
        { 0, "frame_flash" },
        { 1, "fullscreen" },
        { -1, NULL }
};

static GConfValue *
bell_flash_from_widget (GConfPropertyEditor *peditor, const GConfValue *value)
{
        GConfValue *new_value;

        new_value = gconf_value_new (GCONF_VALUE_STRING);
        gconf_value_set_string (new_value,
                                gconf_enum_to_string (bell_flash_enums, gconf_value_get_int (value)));

        return new_value;
}

static GConfValue *
bell_flash_to_widget (GConfPropertyEditor *peditor, const GConfValue *value)
{
        GConfValue *new_value;
        const gchar *str;
        gint val = 2;

        str = (value && (value->type == GCONF_VALUE_STRING)) ? gconf_value_get_string (value) : NULL;

        new_value = gconf_value_new (GCONF_VALUE_INT);
        if (value->type == GCONF_VALUE_STRING) {
                gconf_string_to_enum (bell_flash_enums,
                                      str,
                                      &val);
        }
        gconf_value_set_int (new_value, val);

        return new_value;
}

static void
a11y_notifications_dialog_response_cb (GtkWidget *w, gint response)
{
	if (response == GTK_RESPONSE_HELP) {

	}
	else {
		gtk_widget_destroy (w);
	}
}
static void
notifications_button_clicked_cb (GtkWidget *button, GladeXML *dialog)
{
	GtkWidget *w;

	notifications_dialog = glade_xml_new (GNOMECC_GLADE_DIR GLADE_FILE,
					      "a11y_notifications_dialog", NULL);

	stickykeys_enable_toggled_cb (WID ("stickykeys_enable"), dialog);
	slowkeys_enable_toggled_cb (WID ("slowkeys_enable"), dialog);
	bouncekeys_enable_toggled_cb (WID ("bouncekeys_enable"), dialog);

	w = NWID ("feature_state_change_beep");
	gconf_peditor_new_boolean (NULL,
	                          CONFIG_ROOT "/feature_state_change_beep",
	                          w, NULL);

	w = NWID ("togglekeys_enable");
	gconf_peditor_new_boolean (NULL,
	                          CONFIG_ROOT "/togglekeys_enable",
	                          w, NULL);

	w = NWID ("stickykeys_modifier_beep");
	gconf_peditor_new_boolean (NULL,
	                          CONFIG_ROOT "/stickykeys_modifier_beep",
	                          w, NULL);

	w = NWID ("slowkeys_beep_press");
	gconf_peditor_new_boolean (NULL,
	                          CONFIG_ROOT "/slowkeys_beep_press",
	                          w, NULL);

	w = NWID ("slowkeys_beep_accept");
	gconf_peditor_new_boolean (NULL,
	                          CONFIG_ROOT "/slowkeys_beep_accept",
	                          w, NULL);

	w = NWID ("slowkeys_beep_reject");
	gconf_peditor_new_boolean (NULL,
	                          CONFIG_ROOT "/slowkeys_beep_reject",
	                          w, NULL);

	w = NWID ("bouncekeys_beep_reject");
	gconf_peditor_new_boolean (NULL,
	                          CONFIG_ROOT "/bouncekeys_beep_reject",
	                          w, NULL);

	w = NWID ("visual_bell_enable");
	gconf_peditor_new_boolean (NULL, 
				   "/apps/metacity/general/visual_bell",
				   w, NULL);
        g_signal_connect (w, "toggled",
                          G_CALLBACK (visual_bell_enable_toggled_cb), dialog);
        visual_bell_enable_toggled_cb (w, dialog);

       gconf_peditor_new_select_radio (NULL,
                                        "/apps/metacity/general/visual_bell_type",
                                        gtk_radio_button_get_group (GTK_RADIO_BUTTON (NWID ("visual_bell_titlebar"))),
                                        "conv-to-widget-cb", bell_flash_to_widget,
                                        "conv-from-widget-cb", bell_flash_from_widget,
                                        NULL);

	w = NWID ("a11y_notifications_dialog");
	gtk_window_set_transient_for (GTK_WINDOW (w),
	                              GTK_WINDOW (WID ("keyboard_dialog")));
	g_signal_connect (w, "response",
			  G_CALLBACK (a11y_notifications_dialog_response_cb), NULL);

	gtk_dialog_run (GTK_DIALOG (w));

	g_object_unref (notifications_dialog);
	notifications_dialog = NULL;
}

static void
mousekeys_enable_toggled_cb (GtkWidget *w, GladeXML *dialog)
{
	gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	gtk_widget_set_sensitive (WID ("mousekeys_table"), active);
}

static GConfValue *
mousekeys_accel_time_to_widget (GConfPropertyEditor *peditor, const GConfValue *value)
{
	GtkAdjustment *adjustment;
	gdouble range_upper;
	GConfValue *new_value;

	adjustment = GTK_ADJUSTMENT (gconf_property_editor_get_ui_control (peditor));
	g_object_get (adjustment,
	              "upper", &range_upper,
	              NULL);

	new_value = gconf_value_new (GCONF_VALUE_INT);
	gconf_value_set_int (new_value, MAX (0, ((int) range_upper) - gconf_value_get_int (value)));

	return new_value;
}

static GConfValue *
mousekeys_accel_time_from_widget (GConfPropertyEditor *peditor, const GConfValue *value)
{
	GtkAdjustment *adjustment;
	gdouble range_value, range_upper;
	GConfValue *new_value;

	adjustment = GTK_ADJUSTMENT (gconf_property_editor_get_ui_control (peditor));
	g_object_get (adjustment,
	              "value", &range_value,
	              "upper", &range_upper,
	              NULL);

	new_value = gconf_value_new (GCONF_VALUE_INT);
	gconf_value_set_int (new_value, (int) range_upper - range_value);

	return new_value;
}

void
setup_a11y_tabs (GladeXML *dialog, GConfChangeSet *changeset)
{
	GConfClient *client;
	GtkWidget *w;

	client = gconf_client_get_default ();
	gconf_client_add_dir (client, CONFIG_ROOT, GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	g_object_unref (client);

	/* Accessibility tab */

	w = WID ("master_enable");
	gconf_peditor_new_boolean (changeset,
				   CONFIG_ROOT "/enable",
				   w, NULL);

	w = WID ("stickykeys_enable");
	gconf_peditor_new_boolean (changeset,
				   CONFIG_ROOT "/stickykeys_enable",
				   w, NULL);
	g_signal_connect (w, "toggled",
			  G_CALLBACK (stickykeys_enable_toggled_cb), dialog);
	stickykeys_enable_toggled_cb (w, dialog);

	w = WID ("stickykeys_two_key_off");
	gconf_peditor_new_boolean (changeset,
				   CONFIG_ROOT "/stickykeys_two_key_off",
				   w, NULL);

	w = WID ("slowkeys_enable");
	gconf_peditor_new_boolean (changeset,
				   CONFIG_ROOT "/slowkeys_enable",
				   w, NULL);
	g_signal_connect (w, "toggled",
			  G_CALLBACK (slowkeys_enable_toggled_cb), dialog);
	slowkeys_enable_toggled_cb (w, dialog);

	w = WID ("bouncekeys_enable");
	gconf_peditor_new_boolean (changeset,
				   CONFIG_ROOT "/bouncekeys_enable",
				   w, NULL);
	g_signal_connect (w, "toggled",
			  G_CALLBACK (bouncekeys_enable_toggled_cb), dialog);
	bouncekeys_enable_toggled_cb (w, dialog);

	gconf_peditor_new_numeric_range (changeset,
					 CONFIG_ROOT "/slowkeys_delay",
					 WID ("slowkeys_delay_slide"), NULL);
	gconf_peditor_new_numeric_range (changeset,
					 CONFIG_ROOT "/bouncekeys_delay",
					 WID ("bouncekeys_delay_slide"), NULL);

	w = WID ("notifications_button");
	g_signal_connect (w, "clicked",
			  G_CALLBACK (notifications_button_clicked_cb), dialog);

	/* Mouse Keys tab */

	w = WID ("mousekeys_enable");
	gconf_peditor_new_boolean (changeset,
				   CONFIG_ROOT "/mousekeys_enable",
				   w, NULL);
	g_signal_connect (w, "toggled",
			  G_CALLBACK (mousekeys_enable_toggled_cb), dialog);
	mousekeys_enable_toggled_cb (w, dialog);

	gconf_peditor_new_numeric_range (changeset,
					 CONFIG_ROOT "/mousekeys_accel_time",
					 WID ("mousekeys_accel_time_slide"),
					 "conv-to-widget-cb", mousekeys_accel_time_to_widget,
					 "conv-from-widget-cb", mousekeys_accel_time_from_widget,
					 NULL);
	gconf_peditor_new_numeric_range (changeset,
					 CONFIG_ROOT "/mousekeys_max_speed",
					 WID ("mousekeys_max_speed_slide"), NULL);
	gconf_peditor_new_numeric_range (changeset,
					 CONFIG_ROOT "/mousekeys_init_delay",
					 WID ("mousekeys_init_delay_slide"), NULL);
}
