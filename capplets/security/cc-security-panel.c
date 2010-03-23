/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* cc-security-panel.c
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

/* TODO:
 * * there is currently no indication that the current passwd is being
 *   checked when unfocusing the entry. A busy-interactive cursor might
 *   work
 *
 * NOTES:
 * * currently uses PasswdHandler from accounts-dialog/users-admin/
 *   gnome-about-me. This is quite horrible and seems to break in some 
 *   cases. Using something like accountsservice might be a better long
 *   term solution.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gconf/gconf-client.h>
#include <gtk/gtk.h>

#include "capplet-util.h"
#include "run-passwd.h"
#include "cc-security-panel.h"

#define SCREEN_LOCK_DIR "/apps/gnome-screensaver"
#define SCREEN_LOCK_KEY SCREEN_LOCK_DIR"/lock_enabled"

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_SECURITY_PANEL, CcSecurityPanelPrivate))

#define GET_WIDGET(b,s) GTK_WIDGET (gtk_builder_get_object (b, s))

struct CcSecurityPanelPrivate {
        GtkWidget *password_toggle;
        guint toggle_id;

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
        GConfClient *gconf;
};

G_DEFINE_DYNAMIC_TYPE (CcSecurityPanel, cc_security_panel, CC_TYPE_PANEL)

static void
cc_security_panel_update_sensitivity (CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);
        gboolean ok = FALSE;

        if (priv->current_is_valid) {
                char const *new, *verify;

                new = gtk_entry_get_text (GTK_ENTRY (priv->new_entry));
                verify = gtk_entry_get_text (GTK_ENTRY (priv->verify_entry));
                if (strlen (new) != 0 &&
                    strlen (verify) != 0 &&
                    strcmp (new, verify) == 0) {
                        ok = TRUE;
                }
        }
        gtk_widget_set_sensitive (priv->save_button, ok);
}

static void
password_authenticated_cb (PasswdHandler   *passwd_handler,
                           GError          *error,
                           CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);

        if (error) {
                priv->current_is_valid = FALSE;
                gtk_widget_show (priv->current_info_bar);
        } else {
                priv->current_is_valid = TRUE;
                gtk_widget_hide (priv->current_info_bar);
                if (priv->grab_after_validation &&
                    GTK_WIDGET_HAS_FOCUS (priv->current_entry)) {
                        gtk_widget_grab_focus (priv->new_entry);
                }
        }
        priv->grab_after_validation = FALSE;

        cc_security_panel_update_sensitivity (panel);
}

static void
cc_security_panel_validate_current_password (CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);
        const char *text;

        text = gtk_entry_get_text (GTK_ENTRY (priv->current_entry));
        if (strlen (text) > 0) {
                passwd_authenticate (priv->passwd_handler, text,
                                     (PasswdCallback)password_authenticated_cb,
                                     panel);
        }   
}

static gboolean
cc_security_panel_verify_new_password (CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);
        char const *new, *verify;

        new = gtk_entry_get_text (GTK_ENTRY (priv->new_entry));
        verify = gtk_entry_get_text (GTK_ENTRY (priv->verify_entry));
        if (strlen (new) == 0 || strlen (verify) == 0) {
                gtk_widget_hide (priv->verify_info_bar);
                return FALSE;
        } else if (strcmp (new, verify) == 0) {
                gtk_widget_hide (priv->verify_info_bar);
                return TRUE;
        } else {
                gtk_label_set_text (GTK_LABEL (priv->verify_info_label),
                                    _("Sorry, the passwords do not match"));
                gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->verify_info_bar),
                                               GTK_MESSAGE_WARNING);
                gtk_widget_show (priv->verify_info_bar);
                return FALSE;
        }
}

static void
password_changed_cb (PasswdHandler   *passwd_handler,
                     GError          *error,
                     CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);

       gtk_widget_set_sensitive (priv->current_entry, TRUE);
       gtk_widget_set_sensitive (priv->new_entry, TRUE);
       gtk_widget_set_sensitive (priv->verify_entry, TRUE);

        if (!error) {
                gtk_entry_set_text (GTK_ENTRY (priv->current_entry), "");
                gtk_entry_set_text (GTK_ENTRY (priv->new_entry), "");
                gtk_entry_set_text (GTK_ENTRY (priv->verify_entry), "");

                gtk_label_set_text (GTK_LABEL (priv->verify_info_label),
                                    _("The new password is now saved"));
                gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->verify_info_bar),
                                               GTK_MESSAGE_INFO);
                gtk_widget_show (priv->verify_info_bar);
        } else {
                switch (error->code) {
                case PASSWD_ERROR_REJECTED:
                        gtk_entry_set_text (GTK_ENTRY (priv->new_entry), "");
                        gtk_entry_set_text (GTK_ENTRY (priv->verify_entry), "");
                        gtk_widget_grab_focus (priv->new_entry);

                        gtk_label_set_text (GTK_LABEL (priv->verify_info_label),
                                            error->message);
                        gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->verify_info_bar),
                                                       GTK_MESSAGE_WARNING);
                        gtk_widget_show (priv->verify_info_bar);
                        break;

                case PASSWD_ERROR_AUTH_FAILED:
                        gtk_entry_set_text (GTK_ENTRY (priv->current_entry), "");
                        gtk_widget_grab_focus (priv->current_entry);

                        gtk_widget_show (priv->current_info_bar);
                        break;

                default:
                        g_warning ("Password change failed: %s", error->message);

                        gtk_widget_set_sensitive (priv->save_button, TRUE);

                        gtk_label_set_text (GTK_LABEL (priv->verify_info_label),
                                            _("Sorry, the password could not be changed"));
                        gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->verify_info_bar),
                                                       GTK_MESSAGE_WARNING);
                        gtk_widget_show (priv->verify_info_bar);
                }
        }

        /* PasswdHandler seems to break in interesting ways after this.
         * Fixing it is not trivial, working around... */
        passwd_destroy (priv->passwd_handler);
        priv->passwd_handler = passwd_init ();
        cc_security_panel_validate_current_password (panel);
}

static void
cc_security_panel_change_password (CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);

        const char *new;

        gtk_widget_set_sensitive (priv->current_entry, FALSE);
        gtk_widget_set_sensitive (priv->new_entry, FALSE);
        gtk_widget_set_sensitive (priv->verify_entry, FALSE);
        gtk_widget_set_sensitive (priv->save_button, FALSE);

        new = gtk_entry_get_text (GTK_ENTRY (priv->new_entry));
        passwd_change_password (priv->passwd_handler, new,
                                (PasswdCallback)password_changed_cb, panel);
}

static void
cc_security_panel_update_password_toggle (CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);
        GError *error = NULL;
        gboolean lock = FALSE;

        lock = gconf_client_get_bool (priv->gconf, SCREEN_LOCK_KEY, &error);
        if (error) {
                g_warning ("Could not read key %s: %s",
                           SCREEN_LOCK_KEY, error->message);
                g_error_free (error);
        }

        g_signal_handler_block (priv->password_toggle, priv->toggle_id);
        g_object_set (priv->password_toggle,
                      "active", lock,
                      NULL);
        g_signal_handler_unblock (priv->password_toggle, priv->toggle_id);

}

static void
current_entry_activate (GtkEntry *entry, CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);

        priv->grab_after_validation = TRUE;
        cc_security_panel_validate_current_password (panel);
}

static gboolean
current_entry_focus_out (GtkEntry        *entry,
                         GdkEventFocus   *event,
                         CcSecurityPanel *panel)
{
        cc_security_panel_validate_current_password (panel);
        return FALSE;
}

static void
current_entry_notify_text (GtkEntry        *entry,
                           GParamSpec      *pspec,
                           CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);

        gtk_widget_hide (priv->current_info_bar);
        priv->current_is_valid = FALSE;

        cc_security_panel_update_sensitivity (panel);
}

static void
new_entry_activate (GtkEntry *entry, CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);

        cc_security_panel_verify_new_password (panel);
        gtk_widget_grab_focus (priv->verify_entry);
}

static void
verify_entry_activate (GtkEntry *entry, CcSecurityPanel *panel)
{
        if (cc_security_panel_verify_new_password (panel)) {
                cc_security_panel_change_password (panel);
        }
}

static gboolean
new_or_verify_entry_focus_out (GtkEntry        *entry,
                               GdkEventFocus   *event,
                               CcSecurityPanel *panel)
{
        if (strlen (gtk_entry_get_text (GTK_ENTRY (entry))) > 0) {
                cc_security_panel_verify_new_password (panel);
        }
        return FALSE;
}

static void
new_or_verify_entry_notify_text (GtkEntry     *entry,
                                 GParamSpec   *pspec,
                                 CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);

        gtk_widget_hide (priv->verify_info_bar);
        cc_security_panel_update_sensitivity (panel);
}

static void
password_toggle_notify_active (GtkWidget    *widget,
                               GParamSpec   *pspec,
                               CcSecurityPanel *panel)
{
        char *lockfile;
        gboolean lock;

        g_object_get (widget, "active", &lock, NULL);
        gconf_client_set_bool (panel->priv->gconf,
                               SCREEN_LOCK_KEY, lock,
                               NULL);

        /* touch or remove a file to set lock-on-boot status */
        lockfile = g_strdup_printf ("%s/lock-screen", g_get_user_config_dir ());
        if (lock) {
                g_file_set_contents (lockfile, "", -1, NULL);
        } else {
                g_remove (lockfile);
        }
        g_free (lockfile);
}

static void
save_button_clicked (GtkButton *button, CcSecurityPanel *panel)
{
        cc_security_panel_change_password (panel);
}

static void
gconf_notify (GConfClient *gconf,
              guint id,
              GConfEntry *entry,
              CcSecurityPanel *panel)
{
        cc_security_panel_update_password_toggle (panel);
}

static void
cc_security_panel_setup_panel (CcSecurityPanel *panel)
{
        CcSecurityPanelPrivate *priv = GET_PRIVATE (panel);
        GtkWidget *main_window, *box, *c_area, *label;
        GtkBuilder *builder;
        GError *error = NULL;

        builder = gtk_builder_new ();
        gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);

        if (gtk_builder_add_from_file (builder,
                                       UIDIR"/security.ui",
                                       &error) == 0) {
                g_error ("Could not parse UI file: %s", error->message);
                g_error_free (error);
                g_object_unref (builder);
                return;
        }

        priv->gconf = gconf_client_get_default ();
        gconf_client_add_dir (priv->gconf,
                              SCREEN_LOCK_DIR,
                              GCONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);
        gconf_client_notify_add (priv->gconf,
                                 SCREEN_LOCK_KEY,
                                 (GConfClientNotifyFunc) gconf_notify,
                                 panel, NULL, NULL);

        main_window = GET_WIDGET (builder, "main_window");
        gtk_widget_reparent (gtk_bin_get_child (GTK_BIN (main_window)),
                             GTK_WIDGET (panel));

        priv->password_toggle = GET_WIDGET (builder, "password_toggle");
        cc_security_panel_update_password_toggle (panel);
        priv->toggle_id = g_signal_connect (priv->password_toggle,
                                            "notify::active",
                                            G_CALLBACK (password_toggle_notify_active),
                                            panel);

        priv->current_entry = GET_WIDGET (builder, "current_entry");
        g_signal_connect (priv->current_entry, "activate",
                          G_CALLBACK (current_entry_activate), panel);
        g_signal_connect (priv->current_entry, "focus-out-event",
                          G_CALLBACK (current_entry_focus_out), panel);
        g_signal_connect (priv->current_entry, "notify::text",
                          G_CALLBACK (current_entry_notify_text), panel);

        /* no infobar in glade yet... */
        box = GET_WIDGET (builder, "current_warning_box");
        priv->current_info_bar = gtk_info_bar_new ();
        gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->current_info_bar),
                                       GTK_MESSAGE_WARNING);
        gtk_box_pack_start_defaults (GTK_BOX (box), priv->current_info_bar);
        label = gtk_label_new (_("Sorry, that password is incorrect"));
        gtk_widget_show (label);
        c_area = gtk_info_bar_get_content_area (
                GTK_INFO_BAR (priv->current_info_bar));
        gtk_container_add (GTK_CONTAINER (c_area), label);

        box = GET_WIDGET (builder, "verify_warning_box");
        priv->verify_info_bar = gtk_info_bar_new ();
        gtk_box_pack_start_defaults (GTK_BOX (box), priv->verify_info_bar);
        priv->verify_info_label = gtk_label_new ("");
        gtk_widget_show (priv->verify_info_label);
        c_area = gtk_info_bar_get_content_area (
                GTK_INFO_BAR (priv->verify_info_bar));
        gtk_container_add (GTK_CONTAINER (c_area), priv->verify_info_label);

        priv->new_entry = GET_WIDGET (builder, "new_entry");
        g_signal_connect (priv->new_entry, "activate",
                          G_CALLBACK (new_entry_activate), panel);
        g_signal_connect (priv->new_entry, "focus-out-event",
                          G_CALLBACK (new_or_verify_entry_focus_out), panel);
        g_signal_connect (priv->new_entry, "notify::text",
                          G_CALLBACK (new_or_verify_entry_notify_text), panel);

        priv->verify_entry = GET_WIDGET (builder, "verify_entry");
        g_signal_connect (priv->verify_entry, "activate",
                          G_CALLBACK (verify_entry_activate), panel);
        g_signal_connect (priv->verify_entry, "focus-out-event",
                          G_CALLBACK (new_or_verify_entry_focus_out), panel);
        g_signal_connect (priv->verify_entry, "notify::text",
                          G_CALLBACK (new_or_verify_entry_notify_text), panel);

        priv->save_button = GET_WIDGET (builder, "save_button");
        g_signal_connect (priv->save_button, "clicked",
                          G_CALLBACK (save_button_clicked), panel);

        g_object_unref (builder);

        priv->passwd_handler = passwd_init ();
}

static GObject *
cc_security_panel_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        CcSecurityPanel *panel;

        panel = CC_SECURITY_PANEL (G_OBJECT_CLASS (cc_security_panel_parent_class)->constructor
                                   (type, n_construct_properties, construct_properties));

        g_object_set (panel,
                      "display-name", _("Security"),
                      "id", "security.desktop",
                      NULL);

        cc_security_panel_setup_panel (panel);

        return G_OBJECT (panel);
}


static void
cc_security_panel_class_finalize (CcSecurityPanelClass *klass)
{
}

static void
cc_security_panel_init (CcSecurityPanel *panel)
{
        panel->priv = GET_PRIVATE (panel);
}

static void
cc_security_panel_dispose (GObject *object)
{
        CcSecurityPanel *panel;
        CcSecurityPanelPrivate *priv;

        g_return_if_fail (object != NULL);
        g_return_if_fail (CC_IS_SECURITY_PANEL (object));

        panel = CC_SECURITY_PANEL (object);
        priv = GET_PRIVATE (panel);

        if (priv->passwd_handler) {
                passwd_destroy (priv->passwd_handler);
                priv->passwd_handler = NULL;
        }

        if (priv->gconf) {
                g_object_unref (priv->gconf);
                priv->gconf = NULL;
        }
}

static void
cc_security_panel_finalize (GObject *object)
{
        CcSecurityPanel *panel;

        g_return_if_fail (object != NULL);
        g_return_if_fail (CC_IS_SECURITY_PANEL (object));

        panel = CC_SECURITY_PANEL (object);

        g_return_if_fail (panel->priv != NULL);

        G_OBJECT_CLASS (cc_security_panel_parent_class)->finalize (object);
}

static void
cc_security_panel_class_init (CcSecurityPanelClass *klass)
{
        GObjectClass  *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = cc_security_panel_constructor;
        object_class->finalize = cc_security_panel_finalize;
        object_class->dispose = cc_security_panel_dispose;

        g_type_class_add_private (klass, sizeof (CcSecurityPanelPrivate));
}

void
cc_security_panel_register (GIOModule *module)
{
        cc_security_panel_register_type (G_TYPE_MODULE (module));
        g_io_extension_point_implement (CC_PANEL_EXTENSION_POINT_NAME,
                                        CC_TYPE_SECURITY_PANEL,
                                        "security",
                                        10);
}
