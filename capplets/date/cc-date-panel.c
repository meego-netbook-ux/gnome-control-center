/*
 * date-panels
 * Copyright (C) 2010 Intel Corporation
 * 
 * date-panels is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * date-panels is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cc-date-panel.h"

G_DEFINE_DYNAMIC_TYPE (CcDatePanel, cc_date_panel, CC_TYPE_PANEL);

static void
map (GtkWidget       *socket)
{
  gchar *command;
  GError *err = NULL;
  static GdkNativeWindow win = 0;
  GdkNativeWindow new_win;
  
  g_debug ("map");

  new_win = gtk_socket_get_id (GTK_SOCKET (socket));

  /* map is called multiple times */
  if (new_win == win)
    return;
  win = new_win;

  command = g_strdup_printf ("system-config-date --socket %d", win);


  g_debug ("Running %s", command);
  g_spawn_command_line_async (command, &err);

  if (err)
    {
      g_warning ("Error loading system-config-date: %s", err->message);
      g_error_free (err);
    }
}

static gboolean
plug_removed (GtkSocket *socket,
              CcPanel   *panel)
{
  CcShell *shell;

  shell = cc_panel_get_shell (panel);

  cc_shell_set_panel (shell, NULL);

  return FALSE;
}

static void
active_changed_cb (CcDatePanel *panel,
                   gboolean     is_active,
                   gpointer    *data)
{
  if (is_active)
    {
      GtkWidget *viewport;

      panel->scrolled = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (panel->scrolled),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
      gtk_container_add (GTK_CONTAINER (panel), panel->scrolled);

      viewport = gtk_viewport_new (NULL, NULL);
      gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
      gtk_container_add (GTK_CONTAINER (panel->scrolled), viewport);

      panel->socket = gtk_socket_new ();
      gtk_container_add (GTK_CONTAINER (viewport), panel->socket);


      g_signal_connect (panel->socket, "map", G_CALLBACK (map), NULL);
      g_signal_connect (panel->socket, "plug-removed",
                        G_CALLBACK (plug_removed), panel);

      gtk_widget_show_all (panel->scrolled);
    }
  else
    {
      gtk_widget_destroy (panel->scrolled);
      panel->socket = NULL;
      panel->scrolled = NULL;
    }

}

static void
cc_date_panel_init (CcDatePanel *object)
{
  g_signal_connect (object, "active-changed", G_CALLBACK (active_changed_cb),
                    NULL);
  object->socket = NULL;
}

static void
cc_date_panel_finalize (GObject *object)
{
  G_OBJECT_CLASS (cc_date_panel_parent_class)->finalize (object);
}

static GObject*
cc_date_panel_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
  CcDatePanel *date_panel;

  date_panel = CC_DATE_PANEL (G_OBJECT_CLASS (cc_date_panel_parent_class)->constructor (type,
                                                                                                  n_construct_properties,
                                                                                                  construct_properties));
  g_object_set (date_panel,
                "display-name", ("Date"),
                "id","system-config-date.desktop",
                NULL);

  return G_OBJECT (date_panel);
}

static void
cc_date_panel_class_init (CcDatePanelClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cc_date_panel_finalize;
  object_class->constructor = cc_date_panel_constructor;
}

static void
cc_date_panel_class_finalize (CcDatePanelClass *klass)
{
}

void
cc_date_panel_register (GIOModule *module)
{
  cc_date_panel_register_type (G_TYPE_MODULE (module));
  g_io_extension_point_implement (CC_PANEL_EXTENSION_POINT_NAME,
                                  CC_TYPE_DATE_PANEL,
                                  "date",
                                  10);
}
