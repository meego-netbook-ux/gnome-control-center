/*
 *  Authors: Rodney Dawes <dobey@ximian.com>
 *
 *  Copyright 2003-2006 Novell, Inc. (www.novell.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#include "appearance.h"
#include "gnome-wp-item.h"
#include <gio/gio.h>
#include <string.h>
#include <libxml/parser.h>

static gboolean gnome_wp_xml_get_bool (const xmlNode * parent,
				       const gchar * prop_name) {
  xmlChar * prop;
  gboolean ret_val = FALSE;

  g_return_val_if_fail (parent != NULL, FALSE);
  g_return_val_if_fail (prop_name != NULL, FALSE);

  prop = xmlGetProp ((xmlNode *) parent, (xmlChar*)prop_name);
  if (prop != NULL) {
    if (!g_ascii_strcasecmp ((gchar *)prop, "true") || !g_ascii_strcasecmp ((gchar *)prop, "1")) {
      ret_val = TRUE;
    } else {
      ret_val = FALSE;
    }
    g_free (prop);
  }

  return ret_val;
}

static void gnome_wp_xml_set_bool (const xmlNode * parent,
				   const xmlChar * prop_name, gboolean value) {
  g_return_if_fail (parent != NULL);
  g_return_if_fail (prop_name != NULL);

  if (value) {
    xmlSetProp ((xmlNode *) parent, prop_name, (xmlChar *)"true");
  } else {
    xmlSetProp ((xmlNode *) parent, prop_name, (xmlChar *)"false");
  }
}

static void gnome_wp_load_legacy (AppearanceData *data) {
  FILE * fp;
  gchar * foo, * filename;

  filename = g_build_filename (g_get_home_dir (), ".gnome2",
			       "wallpapers.list", NULL);

  if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
    if ((fp = fopen (filename, "r")) != NULL) {
      foo = (gchar *) g_malloc (sizeof (gchar) * 4096);
      while (fgets (foo, 4096, fp)) {
	GnomeWPItem * item;

	if (foo[strlen (foo) - 1] == '\n') {
	  foo[strlen (foo) - 1] = '\0';
	}

	item = g_hash_table_lookup (data->wp_hash, foo);
	if (item != NULL) {
	  continue;
	}

	if (!g_file_test (foo, G_FILE_TEST_EXISTS)) {
	  continue;
	}

	item = gnome_wp_item_new (foo, data->wp_hash, data->thumb_factory);
	if (item != NULL && item->fileinfo == NULL) {
	  gnome_wp_item_free (item);
	}
      }
      fclose (fp);
      g_free (foo);
    }
  }

  g_free (filename);
}

static void gnome_wp_xml_load_xml (AppearanceData *data,
				   const gchar * filename) {
  xmlDoc * wplist;
  xmlNode * root, * list, * wpa;
  xmlChar * nodelang;
  const gchar * const * syslangs;
  GdkColor color1, color2;
  gint i;

  wplist = xmlParseFile (filename);

  if (!wplist)
    return;

  syslangs = g_get_language_names ();

  root = xmlDocGetRootElement (wplist);

  for (list = root->children; list != NULL; list = list->next) {
    if (!strcmp ((gchar *)list->name, "wallpaper")) {
      GnomeWPItem * wp;
      gchar *pcolor = NULL, *scolor = NULL;
      gchar *s;
      gboolean have_scale = FALSE, have_shade = FALSE;

      wp = g_new0 (GnomeWPItem, 1);

      wp->deleted = gnome_wp_xml_get_bool (list, "deleted");

      for (wpa = list->children; wpa != NULL; wpa = wpa->next) {
	if (!strcmp ((gchar *)wpa->name, "filename")) {
	  if (wpa->last != NULL && wpa->last->content != NULL) {
	    const char * none = "(none)";
	    gchar *content = g_strstrip ((gchar *)wpa->last->content);

	    if (!strcmp (content, none)) {
#ifdef HAVE_MOBLIN
              continue;
#else
	      wp->filename = g_strdup (content);
#endif
	    } else if (g_utf8_validate (content, -1, NULL) &&
		     g_file_test (content, G_FILE_TEST_EXISTS))
	      wp->filename = g_strdup (content);
	    else
	      wp->filename = g_filename_from_utf8 (content, -1, NULL, NULL, NULL);
	  } else {
	    break;
	  }
	} else if (!strcmp ((gchar *)wpa->name, "name")) {
	  if (wpa->last != NULL && wpa->last->content != NULL) {
	    nodelang = xmlNodeGetLang (wpa->last);

	    if (wp->name == NULL && nodelang == NULL) {
	       wp->name = g_strdup (g_strstrip ((gchar *)wpa->last->content));
            } else {
	       for (i = 0; syslangs[i] != NULL; i++) {
	         if (!strcmp (syslangs[i], (gchar *)nodelang)) {
	           g_free (wp->name);
	           wp->name = g_strdup (g_strstrip ((gchar *)wpa->last->content));
	           break;
	         }
	       }
	    }

	    xmlFree (nodelang);
	  } else {
	    break;
	  }
	} else if (!strcmp ((gchar *)wpa->name, "options")) {
	  if (wpa->last != NULL) {
	    wp->options = wp_item_string_to_option (g_strstrip ((gchar *)wpa->last->content));
	    have_scale = TRUE;
	  }
	} else if (!strcmp ((gchar *)wpa->name, "shade_type")) {
	  if (wpa->last != NULL) {
	    wp->shade_type = wp_item_string_to_shading (g_strstrip ((gchar *)wpa->last->content));
	    have_shade = TRUE;
	  }
	} else if (!strcmp ((gchar *)wpa->name, "pcolor")) {
	  if (wpa->last != NULL) {
	    pcolor = g_strdup (g_strstrip ((gchar *)wpa->last->content));
	  }
	} else if (!strcmp ((gchar *)wpa->name, "scolor")) {
	  if (wpa->last != NULL) {
	    scolor = g_strdup (g_strstrip ((gchar *)wpa->last->content));
	  }
	} else if (!strcmp ((gchar *)wpa->name, "text")) {
	  /* Do nothing here, libxml2 is being weird */
	} else {
	  g_warning ("Unknown Tag: %s\n", wpa->name);
	}
      }

      /* Make sure we don't already have this one and that filename exists */
      if (wp->filename == NULL ||
	  g_hash_table_lookup (data->wp_hash, wp->filename) != NULL) {

	gnome_wp_item_free (wp);
	g_free (pcolor);
	g_free (scolor);
	continue;
      }

      /* Verify the colors and alloc some GdkColors here */
      if (!have_scale) {
        s = gconf_client_get_string (data->client, WP_OPTIONS_KEY, NULL);
        wp->options = wp_item_string_to_option (s);
        g_free (s);
      }

      if (!have_shade) {
        s = gconf_client_get_string (data->client, WP_SHADING_KEY, NULL);
        wp->shade_type = wp_item_string_to_shading (s);
        g_free (s);
      }

      if (pcolor == NULL) {
	pcolor = gconf_client_get_string (data->client,
					  WP_PCOLOR_KEY, NULL);
      }
      if (scolor == NULL) {
	scolor = gconf_client_get_string (data->client,
					  WP_SCOLOR_KEY, NULL);
      }
      gdk_color_parse (pcolor, &color1);
      gdk_color_parse (scolor, &color2);
      g_free (pcolor);
      g_free (scolor);

      wp->pcolor = gdk_color_copy (&color1);
      wp->scolor = gdk_color_copy (&color2);

      if ((wp->filename != NULL &&
	   g_file_test (wp->filename, G_FILE_TEST_EXISTS)) ||
	  !strcmp (wp->filename, "(none)")) {
	wp->fileinfo = gnome_wp_info_new (wp->filename, data->thumb_factory);

	if (wp->name == NULL || !strcmp (wp->filename, "(none)")) {
	  g_free (wp->name);
	  wp->name = g_strdup (wp->fileinfo->name);
	}

	gnome_wp_item_update_description (wp);
	g_hash_table_insert (data->wp_hash, wp->filename, wp);
        gnome_wp_item_ensure_gnome_bg (wp);
      } else {
	gnome_wp_item_free (wp);
        wp = NULL;
      }
    }
  }
  xmlFreeDoc (wplist);
}

static void gnome_wp_file_changed (GFileMonitor *monitor,
                                   GFile *file,
                                   GFile *other_file,
                                   GFileMonitorEvent event_type,
                                   AppearanceData *data) {
  gchar * filename;

  switch (event_type) {
  case G_FILE_MONITOR_EVENT_CHANGED:
  case G_FILE_MONITOR_EVENT_CREATED:
    filename = g_file_get_path (file);
    gnome_wp_xml_load_xml (data, filename);
    g_free (filename);
    break;
  default:
    break;
  }
}

static void gnome_wp_xml_add_monitor (GFile *directory,
                                      AppearanceData *data) {
  GFileMonitor *monitor;
  GError *error = NULL;

  monitor = g_file_monitor_directory (directory,
                                      G_FILE_MONITOR_NONE,
                                      NULL,
                                      &error);
  if (error != NULL) {
    gchar *path;

    path = g_file_get_parse_name (directory);
    g_warning ("Unable to monitor directory %s: %s",
               path, error->message);
    g_error_free (error);
    g_free (path);
    return;
  }

  g_signal_connect (monitor, "changed",
                    G_CALLBACK (gnome_wp_file_changed),
                    data);
}

static void gnome_wp_xml_load_from_dir (const gchar *path,
                                        AppearanceData *data) {
  GFile *directory;
  GFileEnumerator *enumerator;
  GError *error = NULL;
  GFileInfo *info;

  if (!g_file_test (path, G_FILE_TEST_IS_DIR)) {
    return;
  }

  directory = g_file_new_for_path (path);
  enumerator = g_file_enumerate_children (directory,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL,
                                          &error);
  if (error != NULL) {
    g_warning ("Unable to check directory %s: %s", path, error->message);
    g_error_free (error);
    g_object_unref (directory);
    return;
  }

  while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL))) {
    const gchar *filename;
    gchar *fullpath;

    filename = g_file_info_get_name (info);
    fullpath = g_build_filename (path, filename, NULL);
    g_object_unref (info);

    gnome_wp_xml_load_xml (data, fullpath);
    g_free (fullpath);
  }
  g_file_enumerator_close (enumerator, NULL, NULL);

  gnome_wp_xml_add_monitor (directory, data);

  g_object_unref (directory);
}

void gnome_wp_xml_load_list (AppearanceData *data) {
  const gchar * xdgdirslist;
  gchar * wpdbfile;
  gchar ** xdgdirs;
  gint i;

  wpdbfile = g_build_filename (g_get_home_dir (),
			       ".gnome2",
			       "backgrounds.xml",
			       NULL);

  if (g_file_test (wpdbfile, G_FILE_TEST_EXISTS)) {
    gnome_wp_xml_load_xml (data, wpdbfile);
  } else {
    g_free (wpdbfile);
    wpdbfile = g_build_filename (g_get_home_dir (),
				 ".gnome2",
				 "wp-list.xml",
				 NULL);
    if (g_file_test (wpdbfile, G_FILE_TEST_EXISTS)) {
      gnome_wp_xml_load_xml (data, wpdbfile);
    }
  }
  g_free (wpdbfile);

  xdgdirslist = g_getenv ("XDG_DATA_DIRS");
  if (xdgdirslist == NULL || strcmp (xdgdirslist, "") == 0)
    xdgdirslist = "/usr/local/share:/usr/share";

  xdgdirs = g_strsplit (xdgdirslist, ":", -1);
  for (i = 0; xdgdirs && xdgdirs[i]; i++) {
    gchar * datadir;

    datadir = g_build_filename (xdgdirs[i], "gnome-background-properties",
				NULL);
    gnome_wp_xml_load_from_dir (datadir, data);
    g_free (datadir);
  }
  g_strfreev (xdgdirs);

  gnome_wp_xml_load_from_dir (WALLPAPER_DATADIR, data);

  gnome_wp_load_legacy (data);
}

static void gnome_wp_list_flatten (const gchar * key, GnomeWPItem * item,
				   GSList ** list) {
  g_return_if_fail (key != NULL);
  g_return_if_fail (item != NULL);

  *list = g_slist_prepend (*list, item);
}

void gnome_wp_xml_save_list (AppearanceData *data) {
  xmlDoc * wplist;
  xmlNode * root, * wallpaper, * item;
  GSList * list = NULL;
  gchar * wpfile;

  g_hash_table_foreach (data->wp_hash,
			(GHFunc) gnome_wp_list_flatten, &list);
  g_hash_table_destroy (data->wp_hash);
  list = g_slist_reverse (list);

  wpfile = g_build_filename (g_get_home_dir (),
			     "/.gnome2",
			     "backgrounds.xml",
			     NULL);

  xmlKeepBlanksDefault (0);

  wplist = xmlNewDoc ((xmlChar *)"1.0");
  xmlCreateIntSubset (wplist, (xmlChar *)"wallpapers", NULL, (xmlChar *)"gnome-wp-list.dtd");
  root = xmlNewNode (NULL, (xmlChar *)"wallpapers");
  xmlDocSetRootElement (wplist, root);

  while (list != NULL) {
    GnomeWPItem * wpitem = list->data;
    const char * none = "(none)";
    gchar * filename;
    const gchar * scale, * shade;
    gchar * pcolor, * scolor;

    if (!strcmp (wpitem->filename, none) ||
	(g_utf8_validate (wpitem->filename, -1, NULL) &&
	 g_file_test (wpitem->filename, G_FILE_TEST_EXISTS)))
      filename = g_strdup (wpitem->filename);
    else
      filename = g_filename_to_utf8 (wpitem->filename, -1, NULL, NULL, NULL);

    pcolor = gdk_color_to_string (wpitem->pcolor);
    scolor = gdk_color_to_string (wpitem->scolor);
    scale = wp_item_option_to_string (wpitem->options);
    shade = wp_item_shading_to_string (wpitem->shade_type);

    wallpaper = xmlNewChild (root, NULL, (xmlChar *)"wallpaper", NULL);
    gnome_wp_xml_set_bool (wallpaper, (xmlChar *)"deleted", wpitem->deleted);
    item = xmlNewTextChild (wallpaper, NULL, (xmlChar *)"name", (xmlChar *)wpitem->name);
    item = xmlNewTextChild (wallpaper, NULL, (xmlChar *)"filename", (xmlChar *)filename);
    item = xmlNewTextChild (wallpaper, NULL, (xmlChar *)"options", (xmlChar *)scale);
    item = xmlNewTextChild (wallpaper, NULL, (xmlChar *)"shade_type", (xmlChar *)shade);
    item = xmlNewTextChild (wallpaper, NULL, (xmlChar *)"pcolor", (xmlChar *)pcolor);
    item = xmlNewTextChild (wallpaper, NULL, (xmlChar *)"scolor", (xmlChar *)scolor);
    g_free (pcolor);
    g_free (scolor);
    g_free (filename);

    list = g_slist_delete_link (list, list);
    gnome_wp_item_free (wpitem);
  }
  xmlSaveFormatFile (wpfile, wplist, 1);
  xmlFreeDoc (wplist);
  g_free (wpfile);
}
