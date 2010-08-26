/*
 * Copyright (c) 2009, 2010 Intel, Inc.
 * Copyright (c) 2010 Red Hat, Inc.
 *
 * The Control Center is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * The Control Center is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with the Control Center; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Thomas Wood <thos@gnome.org>
 */

#include "config.h"

#include "control-center.h"

#include <unique/unique.h>
#include <glib/gi18n.h>

#define CC_SHELL_RAISE_COMMAND 1

static UniqueResponse
message_received (UniqueApp         *app,
                  gint               command,
                  UniqueMessageData *message_data,
                  guint              time_,
                  CcShell           *shell)
{
  const gchar *id;
  gsize len;

  control_center_present (CONTROL_CENTER (shell));

  id = (gchar*) unique_message_data_get (message_data, &len);

  if (id)
    {
      cc_shell_set_panel (shell, id);
    }

  return GTK_RESPONSE_OK;
}

int
main (int argc, char **argv)
{
  UniqueApp *unique;
  CcShell *shell;

  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  unique = unique_app_new_with_commands ("org.gnome.ControlCenter",
                                         NULL,
                                         "raise",
                                         CC_SHELL_RAISE_COMMAND,
                                         NULL);

  if (unique_app_is_running (unique))
    {
      UniqueMessageData *data;
      if (argc == 2)
        {
          data = unique_message_data_new ();
          unique_message_data_set (data, (guchar*) argv[1],
                                   strlen(argv[1]) + 1);
        }
      else
        data = NULL;

      unique_app_send_message (unique, 1, data);

      gdk_notify_startup_complete ();
      return 0;
    }

  shell = control_center_new ();

  g_signal_connect (unique, "message-received", G_CALLBACK (message_received),
                    shell);


  if (argc == 2)
    {
      gchar *start_id;

      start_id = argv[1];

      cc_shell_set_panel (shell, start_id);
    }

  gtk_main ();

  return 0;
}
