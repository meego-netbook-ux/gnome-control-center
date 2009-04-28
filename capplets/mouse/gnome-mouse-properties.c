/* -*- mode: c; style: linux -*- */

/* mouse-properties-capplet.c
 * Copyright (C) 2001 Red Hat, Inc.
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Written by: Jonathon Blandford <jrb@redhat.com>,
 *             Bradford Hovinen <hovinen@ximian.com>,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>

#include <glib/gi18n.h>
#include <string.h>
#include <gconf/gconf-client.h>
#include <glade/glade.h>
#include <gdk/gdkx.h>
#include <math.h>

#include "capplet-util.h"
#include "gconf-property-editor.h"
#include "activate-settings-daemon.h"
#include "capplet-stock-icons.h"
#include "gnome-mouse-accessibility.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef HAVE_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif

enum
{
	DOUBLE_CLICK_TEST_OFF,
	DOUBLE_CLICK_TEST_MAYBE,
	DOUBLE_CLICK_TEST_ON
};

#ifdef HAVE_MOBLIN
#define GLADE_FILE	"/gnome-mouse-properties-moblin.glade"
#else
#define GLADE_FILE	"/gnome-mouse-properties.glade"
#endif

/* We use this in at least half a dozen places, so it makes sense just to
 * define the macro */
#define DOUBLE_CLICK_KEY "/desktop/gnome/peripherals/mouse/double_click"

#ifndef HAVE_MOBLIN
/* State in testing the double-click speed. Global for a great deal of
 * convenience
 */
static gint double_click_state = DOUBLE_CLICK_TEST_OFF;

/* normalization routines */
/* All of our scales but double_click are on the range 1->10 as a result, we
 * have a few routines to convert from whatever the gconf key is to our range.
 */
static GConfValue *
double_click_from_gconf (GConfPropertyEditor *peditor, const GConfValue *value)
{
	GConfValue *new_value;

	new_value = gconf_value_new (GCONF_VALUE_INT);
	gconf_value_set_int (new_value, CLAMP ((int) floor ((gconf_value_get_int (value) + 50) / 100.0) * 100, 100, 1000));
	return new_value;
}
#endif

static void
get_default_mouse_info (int *default_numerator, int *default_denominator, int *default_threshold)
{
	int numerator, denominator;
	int threshold;
	int tmp_num, tmp_den, tmp_threshold;

	/* Query X for the default value */
	XGetPointerControl (GDK_DISPLAY (), &numerator, &denominator,
			    &threshold);
	XChangePointerControl (GDK_DISPLAY (), True, True, -1, -1, -1);
	XGetPointerControl (GDK_DISPLAY (), &tmp_num, &tmp_den, &tmp_threshold);
	XChangePointerControl (GDK_DISPLAY (), True, True, numerator, denominator, threshold);

	if (default_numerator)
		*default_numerator = tmp_num;

	if (default_denominator)
		*default_denominator = tmp_den;

	if (default_threshold)
		*default_threshold = tmp_threshold;

}

static GConfValue *
motion_acceleration_from_gconf (GConfPropertyEditor *peditor,
				const GConfValue *value)
{
	GConfValue *new_value;
	gfloat motion_acceleration;

	new_value = gconf_value_new (GCONF_VALUE_FLOAT);

	if (gconf_value_get_float (value) == -1.0) {
		int numerator, denominator;

		get_default_mouse_info (&numerator, &denominator, NULL);

		motion_acceleration = CLAMP ((gfloat)(numerator / denominator), 0.2, 6.0);
	}
	else {
		motion_acceleration = CLAMP (gconf_value_get_float (value), 0.2, 6.0);
	}

	if (motion_acceleration >= 1)
		gconf_value_set_float (new_value, motion_acceleration + 4);
	else
		gconf_value_set_float (new_value, motion_acceleration * 5);

	return new_value;
}

static GConfValue *
motion_acceleration_to_gconf (GConfPropertyEditor *peditor,
			      const GConfValue *value)
{
	GConfValue *new_value;
	gfloat motion_acceleration;

	new_value = gconf_value_new (GCONF_VALUE_FLOAT);
	motion_acceleration = CLAMP (gconf_value_get_float (value), 1.0, 10.0);

	if (motion_acceleration < 5)
		gconf_value_set_float (new_value, motion_acceleration / 5.0);
	else
		gconf_value_set_float (new_value, motion_acceleration - 4);

	return new_value;
}

static GConfValue *
threshold_from_gconf (GConfPropertyEditor *peditor,
		      const GConfValue *value)
{
	GConfValue *new_value;

	new_value = gconf_value_new (GCONF_VALUE_FLOAT);

	if (gconf_value_get_int (value) == -1) {
		int threshold;

		get_default_mouse_info (NULL, NULL, &threshold);
		gconf_value_set_float (new_value, CLAMP (threshold, 1, 10));
	}
	else {
		gconf_value_set_float (new_value, CLAMP (gconf_value_get_int (value), 1, 10));
	}

	return new_value;
}

#ifndef HAVE_MOBLIN
static GConfValue *
drag_threshold_from_gconf (GConfPropertyEditor *peditor,
			   const GConfValue *value)
{
	GConfValue *new_value;

	new_value = gconf_value_new (GCONF_VALUE_FLOAT);

	gconf_value_set_float (new_value, CLAMP (gconf_value_get_int (value), 1, 10));

	return new_value;
}
#endif

/* Double Click handling */

struct test_data_t
{
	gint *timeout_id;
	GtkWidget *image;
};

/* Timeout for the double click test */

#ifndef HAVE_MOBLIN
static gboolean
test_maybe_timeout (struct test_data_t *data)
{
	double_click_state = DOUBLE_CLICK_TEST_OFF;

	gtk_image_set_from_stock (GTK_IMAGE (data->image),
				  MOUSE_DBLCLCK_OFF, mouse_capplet_dblclck_icon_get_size());

	*data->timeout_id = 0;

	return FALSE;
}

/* Callback issued when the user clicks the double click testing area. */

static gboolean
event_box_button_press_event (GtkWidget   *widget,
			      GdkEventButton *event,
			      GConfChangeSet *changeset)
{
	gint                       double_click_time;
	GConfValue                *value;
	static struct test_data_t  data;
	static gint                test_on_timeout_id     = 0;
	static gint                test_maybe_timeout_id  = 0;
	static guint32             double_click_timestamp = 0;
	GtkWidget                 *image;
	GConfClient               *client;

	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;

	image = g_object_get_data (G_OBJECT (widget), "image");

	if (!(changeset && gconf_change_set_check_value (changeset, DOUBLE_CLICK_KEY, &value))) {
		client = gconf_client_get_default();
		double_click_time = gconf_client_get_int (client, DOUBLE_CLICK_KEY, NULL);
		g_object_unref (client);

	} else
		double_click_time = gconf_value_get_int (value);

	if (test_maybe_timeout_id != 0)
		g_source_remove  (test_maybe_timeout_id);
	if (test_on_timeout_id != 0)
		g_source_remove (test_on_timeout_id);

	switch (double_click_state) {
	case DOUBLE_CLICK_TEST_OFF:
		double_click_state = DOUBLE_CLICK_TEST_MAYBE;
		data.image = image;
		data.timeout_id = &test_maybe_timeout_id;
		test_maybe_timeout_id = g_timeout_add (double_click_time, (GtkFunction) test_maybe_timeout, &data);
		break;
	case DOUBLE_CLICK_TEST_MAYBE:
		if (event->time - double_click_timestamp < double_click_time) {
			double_click_state = DOUBLE_CLICK_TEST_ON;
			data.image = image;
			data.timeout_id = &test_on_timeout_id;
			test_on_timeout_id = g_timeout_add (2500, (GtkFunction) test_maybe_timeout, &data);
		}
		break;
	case DOUBLE_CLICK_TEST_ON:
		double_click_state = DOUBLE_CLICK_TEST_OFF;
		break;
	}

	double_click_timestamp = event->time;

	switch (double_click_state) {
	case DOUBLE_CLICK_TEST_ON:
		gtk_image_set_from_stock (GTK_IMAGE (image),
					  MOUSE_DBLCLCK_ON, mouse_capplet_dblclck_icon_get_size());
		break;
	case DOUBLE_CLICK_TEST_MAYBE:
		gtk_image_set_from_stock (GTK_IMAGE (image),
					  MOUSE_DBLCLCK_MAYBE, mouse_capplet_dblclck_icon_get_size());
		break;
	case DOUBLE_CLICK_TEST_OFF:
		gtk_image_set_from_stock (GTK_IMAGE (image),
					  MOUSE_DBLCLCK_OFF, mouse_capplet_dblclck_icon_get_size());
		break;
	}

	return TRUE;
}
#endif

static void
orientation_radio_button_release_event (GtkWidget   *widget,
				        GdkEventButton *event)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
}

static GConfValue *
left_handed_from_gconf (GConfPropertyEditor *peditor,
			const GConfValue *value)
{
	GConfValue *new_value;

	new_value = gconf_value_new (GCONF_VALUE_INT);

	gconf_value_set_int (new_value, gconf_value_get_bool (value));

	return new_value;
}

static GConfValue *
left_handed_to_gconf (GConfPropertyEditor *peditor,
		      const GConfValue *value)
{
	GConfValue *new_value;

	new_value = gconf_value_new (GCONF_VALUE_BOOL);

	gconf_value_set_bool (new_value, gconf_value_get_int (value) == 1);

	return new_value;
}

/* Set up the property editors in the dialog. */
static void
setup_dialog (GladeXML *dialog, GConfChangeSet *changeset)
{
	GtkRadioButton    *radio;
	GObject           *peditor;

	/* Orientation radio buttons */
	radio = GTK_RADIO_BUTTON (WID ("left_handed_radio"));
	peditor = gconf_peditor_new_select_radio
		(changeset, "/desktop/gnome/peripherals/mouse/left_handed", gtk_radio_button_get_group (radio),
		 "conv-to-widget-cb", left_handed_from_gconf,
		 "conv-from-widget-cb", left_handed_to_gconf,
		 NULL);
	g_signal_connect (WID ("right_handed_radio"), "button_release_event",
		G_CALLBACK (orientation_radio_button_release_event), NULL);
	g_signal_connect (WID ("left_handed_radio"), "button_release_event",
		G_CALLBACK (orientation_radio_button_release_event), NULL);

#ifndef HAVE_MOBLIN
	/* Locate pointer toggle */
	peditor = gconf_peditor_new_boolean
		(changeset, "/desktop/gnome/peripherals/mouse/locate_pointer", WID ("locate_pointer_toggle"), NULL);

	/* Double-click time */
	peditor = gconf_peditor_new_numeric_range
		(changeset, DOUBLE_CLICK_KEY, WID ("delay_scale"),
		 "conv-to-widget-cb", double_click_from_gconf,
		 NULL);
	gtk_image_set_from_stock (GTK_IMAGE (WID ("double_click_image")), MOUSE_DBLCLCK_OFF, mouse_capplet_dblclck_icon_get_size ());
	g_object_set_data (G_OBJECT (WID ("double_click_eventbox")), "image", WID ("double_click_image"));
	g_signal_connect (WID ("double_click_eventbox"), "button_press_event",
			  G_CALLBACK (event_box_button_press_event), changeset);
#endif

	/* speed */
      	gconf_peditor_new_numeric_range
		(changeset, "/desktop/gnome/peripherals/mouse/motion_acceleration", WID ("accel_scale"),
		 "conv-to-widget-cb", motion_acceleration_from_gconf,
		 "conv-from-widget-cb", motion_acceleration_to_gconf,
		 NULL);

	gconf_peditor_new_numeric_range
		(changeset, "/desktop/gnome/peripherals/mouse/motion_threshold", WID ("sensitivity_scale"),
		 "conv-to-widget-cb", threshold_from_gconf,
		 NULL);

#ifndef HAVE_MOBLIN
	/* DnD threshold */
	gconf_peditor_new_numeric_range
		(changeset, "/desktop/gnome/peripherals/mouse/drag_threshold", WID ("drag_threshold_scale"),
		 "conv-to-widget-cb", drag_threshold_from_gconf,
		 NULL);
#endif
}

/* Construct the dialog */

static GladeXML *
create_dialog (void)
{
	GladeXML     *dialog;
	GtkSizeGroup *size_group;

	dialog = glade_xml_new (GNOMECC_GLADE_DIR GLADE_FILE, "mouse_properties_dialog", NULL);
	if (!dialog)
		return NULL;

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("acceleration_label"));
	gtk_size_group_add_widget (size_group, WID ("sensitivity_label"));
#ifndef HAVE_MOBLIN
	gtk_size_group_add_widget (size_group, WID ("threshold_label"));
	gtk_size_group_add_widget (size_group, WID ("timeout_label"));
#endif

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("acceleration_fast_label"));
	gtk_size_group_add_widget (size_group, WID ("sensitivity_high_label"));
#ifndef HAVE_MOBLIN
	gtk_size_group_add_widget (size_group, WID ("threshold_large_label"));
	gtk_size_group_add_widget (size_group, WID ("timeout_long_label"));
#endif

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("acceleration_slow_label"));
	gtk_size_group_add_widget (size_group, WID ("sensitivity_low_label"));
#ifndef HAVE_MOBLIN
	gtk_size_group_add_widget (size_group, WID ("threshold_small_label"));
	gtk_size_group_add_widget (size_group, WID ("timeout_short_label"));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("simulated_delay_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_delay_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_threshold_label"));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("simulated_delay_short_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_delay_short_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_threshold_small_label"));

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, WID ("simulated_delay_long_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_delay_long_label"));
	gtk_size_group_add_widget (size_group, WID ("dwell_threshold_large_label"));
#endif

	return dialog;
}

/* Callback issued when a button is clicked on the dialog */

static void
dialog_response_cb (GtkDialog *dialog, gint response_id, GConfChangeSet *changeset)
{
	if (response_id == GTK_RESPONSE_HELP)
		capplet_help (GTK_WINDOW (dialog),
			      "goscustperiph-5");
	else
		gtk_main_quit ();
}

int
main (int argc, char **argv)
{
	GConfClient    *client;
	GladeXML       *dialog;
	GtkWidget      *dialog_win, *w;
	gchar *start_page = NULL;

	GOptionContext *context;
	GOptionEntry cap_options[] = {
		{"show-page", 'p', G_OPTION_FLAG_IN_MAIN,
		 G_OPTION_ARG_STRING,
		 &start_page,
		 /* TRANSLATORS: don't translate the terms in brackets */
		 N_("Specify the name of the page to show (general|accessibility)"),
		 N_("page") },
		{NULL}
	};

	context = g_option_context_new (_("- GNOME Mouse Preferences"));
	g_option_context_add_main_entries (context, cap_options, GETTEXT_PACKAGE);
	capplet_init (context, &argc, &argv);
	
	capplet_init_stock_icons ();

	activate_settings_daemon ();

	client = gconf_client_get_default ();
	gconf_client_add_dir (client, "/desktop/gnome/peripherals/mouse", GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

	dialog = create_dialog ();

	if (dialog) {
		setup_dialog (dialog, NULL);
#ifndef HAVE_MOBLIN
		setup_accessibility (dialog, client);
#endif

		dialog_win = WID ("mouse_properties_dialog");
		g_signal_connect (dialog_win, "response",
				  G_CALLBACK (dialog_response_cb), NULL);

		if (start_page != NULL) {
			gchar *page_name;

			page_name = g_strconcat (start_page, "_vbox", NULL);
			g_free (start_page);

			w = WID (page_name);
			if (w != NULL) {
				GtkNotebook *nb;
				gint pindex;

				nb = GTK_NOTEBOOK (WID ("prefs_widget"));
				pindex = gtk_notebook_page_num (nb, w);
				if (pindex != -1)
					gtk_notebook_set_current_page (nb, pindex);
			}
			g_free (page_name);
		}

		capplet_set_icon (dialog_win, "input-mouse");
		gtk_widget_show (dialog_win);

		gtk_main ();

		g_object_unref (dialog);
	}

	g_object_unref (client);

	return 0;
}
