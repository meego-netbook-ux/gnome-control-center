/*
 * gnome-settings-dbus.h
 *
 * Copyright (C) 2007 Jan Arne Petersen <jap@gnome.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 * USA.
 */

#ifndef __GNOME_SETTINGS_DBUS_H__
#define __GNOME_SETTINGS_DBUS_H__

#include <glib-object.h>

G_BEGIN_DECLS

GObject *gnome_settings_server_get                      (void);
void     gnome_settings_server_media_player_key_pressed (GObject *server, const gchar *key);

G_END_DECLS

#endif