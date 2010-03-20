/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* security.c
 * Copyright (C) 2010 Intel Corporation
 *
 * Written by: Jussi Kukkonen <jku@linux.intel.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>
#include <gtk/gtk.h>

#include "capplet-util.h"
#include "run-passwd.h"

#define SCREEN_LOCK_DIR "/apps/gnome-screensaver"
#define SCREEN_LOCK_KEY SCREEN_LOCK_DIR"/lock_enabled"

#define GET_WIDGET(b,s) GTK_WIDGET (gtk_builder_get_object (b, s))

typedef struct {
        GtkWidget *main_window;
        GtkWidget *password_toggle;

        GtkWidget *current_entry;
        GtkWidget *current_info_bar;
        gboolean grab_after_validation;
        gboolean current_is_valid;

        GtkWidget *new_entry;
        GtkWidget *verify_entry;
        GtkWidget *verify_info_bar;
        GtkWidget *verify_info_label;

        GtkWidget *save_button;

        PasswdHandler *passwd_handler;

        
} SecurityData;


static void
security_update_sensitivity (SecurityData *data)
{
        gboolean ok = FALSE;

        if (data->current_is_valid) {
                char const *new, *verify;

                new = gtk_entry_get_text (GTK_ENTRY (data->new_entry));
                verify = gtk_entry_get_text (GTK_ENTRY (data->verify_entry));
                if (strlen (new) != 0 &&
                    strlen (verify) != 0 &&
                    strcmp (new, verify) == 0) {
                        ok = TRUE;
                }
        }
        gtk_widget_set_sensitive (data->save_button, ok);
}

static void
password_authenticated_cb (PasswdHandler *passwd_handler,
                           GError        *error,
                           SecurityData  *data)
{
        if (error) {
                data->current_is_valid = FALSE;
                gtk_widget_show (data->current_info_bar);
        } else {
                data->current_is_valid = TRUE;
                gtk_widget_hide (data->current_info_bar);
                if (data->grab_after_validation &&
                    GTK_WIDGET_HAS_FOCUS (data->current_entry)) {
                        gtk_widget_grab_focus (data->new_entry);
                }
        }
        data->grab_after_validation = FALSE;

        security_update_sensitivity (data);
}

static void
security_validate_current_password (SecurityData *data)
{
        const char *text;

        text = gtk_entry_get_text (GTK_ENTRY (data->current_entry));
        if (strlen (text) > 0) {
                passwd_authenticate (data->passwd_handler, text,
                                     (PasswdCallback)password_authenticated_cb,
                                     data);
        }   
}

static gboolean
security_verify_new_password (SecurityData *data)
{
        char const *new, *verify;

        new = gtk_entry_get_text (GTK_ENTRY (data->new_entry));
        verify = gtk_entry_get_text (GTK_ENTRY (data->verify_entry));
        if (strlen (new) == 0 || strlen (verify) == 0) {
                gtk_widget_hide (data->verify_info_bar);
                return FALSE;
        } else if (strcmp (new, verify) == 0) {
                gtk_widget_hide (data->verify_info_bar);
                return TRUE;
        } else {
                gtk_label_set_text (GTK_LABEL (data->verify_info_label),
                                    _("Sorry, the passwords do not match"));
                gtk_widget_show (data->verify_info_bar);
                return FALSE;
        }
}

static void
password_changed_cb (PasswdHandler *passwd_handler,
                     GError        *error,
                     SecurityData  *data)
{
        if (!error) {
                /* TODO is this correct ? */
                gtk_main_quit ();
                return;
        }

       gtk_widget_set_sensitive (data->current_entry, TRUE);
       gtk_widget_set_sensitive (data->new_entry, TRUE);
       gtk_widget_set_sensitive (data->verify_entry, TRUE);

        switch (error->code) {
        case PASSWD_ERROR_REJECTED:
                gtk_entry_set_text (GTK_ENTRY (data->new_entry), "");
                gtk_entry_set_text (GTK_ENTRY (data->verify_entry), "");
                gtk_widget_grab_focus (data->new_entry);

                gtk_label_set_text (GTK_LABEL (data->verify_info_label),
                                    error->message);
                gtk_widget_show (data->verify_info_bar);
                break;

        case PASSWD_ERROR_AUTH_FAILED:
                gtk_entry_set_text (GTK_ENTRY (data->current_entry), "");
                gtk_widget_grab_focus (data->current_entry);

                gtk_widget_show (data->current_info_bar);
                break;

        default:
                g_warning ("Password change failed: %s", error->message);
 
                gtk_widget_set_sensitive (data->save_button, TRUE);

                gtk_label_set_text (GTK_LABEL (data->verify_info_label),
                                    _("Sorry, the password could not be changed"));
                gtk_widget_show (data->verify_info_bar);
        }

        /* PasswdHandler seems to break in interesting ways on after errors.
         * Fixing it is not trivial, working around... */
        passwd_destroy (data->passwd_handler);
        data->passwd_handler = passwd_init ();
        security_validate_current_password (data);
}

static void
security_change_password (SecurityData *data)
{
        const char *new;

        gtk_widget_set_sensitive (data->current_entry, FALSE);
        gtk_widget_set_sensitive (data->new_entry, FALSE);
        gtk_widget_set_sensitive (data->verify_entry, FALSE);
        gtk_widget_set_sensitive (data->save_button, FALSE);

        new = gtk_entry_get_text (GTK_ENTRY (data->new_entry));
        passwd_change_password (data->passwd_handler, new,
                                (PasswdCallback)password_changed_cb, data);
}

static void
security_update_password_toggle (SecurityData *data, GConfClient *gconf)
{
        GError *error = NULL;
        gboolean lock = FALSE;

        lock = gconf_client_get_bool (gconf, SCREEN_LOCK_KEY, &error);
        if (error) {
                g_warning ("Could not read key %s: %s",
                           SCREEN_LOCK_KEY, error->message);
                g_error_free (error);
        }

        g_object_set (data->password_toggle,
                      "active", lock,
                      NULL);
}

static void
current_entry_activate (GtkEntry *entry, SecurityData *data)
{
        data->grab_after_validation = TRUE;
        security_validate_current_password (data);
}

static gboolean
current_entry_focus_out (GtkEntry      *entry,
                         GdkEventFocus *event,
                         SecurityData  *data)
{
        security_validate_current_password (data);
        return FALSE;
}

static void
current_entry_notify_text (GtkEntry     *entry,
                           GParamSpec   *pspec,
                           SecurityData *data)
{
        gtk_widget_hide (data->current_info_bar);
        data->current_is_valid = FALSE;

        security_update_sensitivity (data);
}

static void
new_entry_activate (GtkEntry *entry, SecurityData *data)
{
        security_verify_new_password (data);
        gtk_widget_grab_focus (data->verify_entry);
}

static void
verify_entry_activate (GtkEntry *entry, SecurityData *data)
{
        if (security_verify_new_password (data)) {
                security_change_password (data);
        }
}

static gboolean
new_or_verify_entry_focus_out (GtkEntry      *entry,
                               GdkEventFocus *event,
                               SecurityData  *data)
{
        if (strlen (gtk_entry_get_text (GTK_ENTRY (entry))) > 0) {
                security_verify_new_password (data);
        }
        return FALSE;
}

static void
new_or_verify_entry_notify_text (GtkEntry     *entry,
                                 GParamSpec   *pspec,
                                 SecurityData *data)
{
        gtk_widget_hide (data->verify_info_bar);
        security_update_sensitivity (data);
}

static void
password_toggle_notify_active (GtkWidget    *widget,
                               GParamSpec   *pspec,
                               SecurityData *data)
{
        gboolean lock;
        GConfClient *gconf;

        g_object_get (widget, "active", &lock, NULL);
        gconf = gconf_client_get_default ();
        gconf_client_set_bool (gconf,
                               SCREEN_LOCK_KEY, lock,
                               NULL);
        g_object_unref (gconf);
}

static void
save_button_clicked (GtkButton *button, SecurityData *data)
{
        security_change_password (data);
}

static void
gconf_notify (GConfClient *gconf,
              guint id,
              GConfEntry *entry,
              SecurityData *data)
{
        security_update_password_toggle (data, gconf);
}

static SecurityData*
security_new ()
{
        SecurityData *data;
        GtkWidget *box, *c_area, *label;
        GtkBuilder *builder;
        GConfClient *gconf;
        GError *error = NULL;

        builder = gtk_builder_new ();
        gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);

        if (gtk_builder_add_from_file (builder,
                                       UIDIR"/security.ui",
                                       &error) == 0) {
                g_warning ("Could not parse UI file: %s", error->message);
                g_error_free (error);
                g_object_unref (builder);
                return NULL;
        }

        data = g_new0 (SecurityData, 1);    

        gconf = gconf_client_get_default ();
        gconf_client_add_dir (gconf,
                              SCREEN_LOCK_DIR,
                              GCONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);
        gconf_client_notify_add (gconf,
                                 SCREEN_LOCK_DIR,
                                 (GConfClientNotifyFunc) gconf_notify,
                                 data, NULL, NULL);

        data->main_window = GET_WIDGET (builder, "main_window");
        g_object_ref (data->main_window);
        g_signal_connect_swapped (data->main_window, "destroy",
                                  G_CALLBACK (gtk_main_quit), NULL);

        data->password_toggle = GET_WIDGET (builder, "password_toggle");
        security_update_password_toggle (data, gconf);
        g_signal_connect (data->password_toggle, "notify::active",
                          G_CALLBACK (password_toggle_notify_active), data);

        data->current_entry = GET_WIDGET (builder, "current_entry");
        g_signal_connect (data->current_entry, "activate",
                          G_CALLBACK (current_entry_activate), data);
        g_signal_connect (data->current_entry, "focus-out-event",
                          G_CALLBACK (current_entry_focus_out), data);
        g_signal_connect (data->current_entry, "notify::text",
                          G_CALLBACK (current_entry_notify_text), data);

        /* no infobar in glade yet... */
        box = GET_WIDGET (builder, "current_warning_box");
        data->current_info_bar = gtk_info_bar_new ();
        gtk_info_bar_set_message_type (GTK_INFO_BAR (data->current_info_bar),
                                       GTK_MESSAGE_WARNING);
        gtk_box_pack_start_defaults (GTK_BOX (box), data->current_info_bar);
        label = gtk_label_new (_("Sorry, that password is incorrect"));
        gtk_widget_show (label);
        c_area = gtk_info_bar_get_content_area (
                GTK_INFO_BAR (data->current_info_bar));
        gtk_container_add (GTK_CONTAINER (c_area), label);

        box = GET_WIDGET (builder, "verify_warning_box");
        data->verify_info_bar = gtk_info_bar_new ();
        gtk_info_bar_set_message_type (GTK_INFO_BAR (data->verify_info_bar),
                                       GTK_MESSAGE_WARNING);
        gtk_box_pack_start_defaults (GTK_BOX (box), data->verify_info_bar);
        data->verify_info_label = gtk_label_new ("");
        gtk_widget_show (data->verify_info_label);
        c_area = gtk_info_bar_get_content_area (
                GTK_INFO_BAR (data->verify_info_bar));
        gtk_container_add (GTK_CONTAINER (c_area), data->verify_info_label);

        data->new_entry = GET_WIDGET (builder, "new_entry");
        g_signal_connect (data->new_entry, "activate",
                          G_CALLBACK (new_entry_activate), data);
        g_signal_connect (data->new_entry, "focus-out-event",
                          G_CALLBACK (new_or_verify_entry_focus_out), data);
        g_signal_connect (data->new_entry, "notify::text",
                          G_CALLBACK (new_or_verify_entry_notify_text), data);

        data->verify_entry = GET_WIDGET (builder, "verify_entry");
        g_signal_connect (data->verify_entry, "activate",
                          G_CALLBACK (verify_entry_activate), data);
        g_signal_connect (data->verify_entry, "focus-out-event",
                          G_CALLBACK (new_or_verify_entry_focus_out), data);
        g_signal_connect (data->verify_entry, "notify::text",
                          G_CALLBACK (new_or_verify_entry_notify_text), data);

        data->save_button = GET_WIDGET (builder, "save_button");
        g_signal_connect (data->save_button, "clicked",
                          G_CALLBACK (save_button_clicked), data);

        capplet_set_icon (data->main_window, "preferences-system-FIXME");
        gtk_widget_show (data->main_window);

        g_object_unref (builder);
        g_object_unref (gconf);

        data->passwd_handler = passwd_init ();

        return data;
}

static void
security_free (SecurityData *data)
{
        g_signal_handlers_disconnect_by_func (data->main_window,
                                              G_CALLBACK (gtk_main_quit),
                                              NULL);
        gtk_widget_destroy (data->main_window);

        passwd_destroy (data->passwd_handler);

        g_free (data);
}

int
main (int argc, char **argv)
{
        SecurityData *data;

        bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
        bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        gtk_init (&argc, &argv);

        data = security_new ();
        if (!data) {
            return 1;
        }

        gtk_main ();

        security_free (data);
        return 0;
}
