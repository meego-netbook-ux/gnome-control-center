/*
 * Copyright (C) 2010 Intel, Inc
 *
 * Portions from Ubiquity, Copyright (C) 2009 Canonical Ltd.
 * Written by Evan Dandrea <evand@ubuntu.com>
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
 * Author: Thomas Wood <thomas.wood@intel.com>
 *
 */

#include "cc-timezone-map.h"
#include <math.h>
#include "tz.h"

G_DEFINE_TYPE (CcTimezoneMap, cc_timezone_map, GTK_TYPE_WIDGET)

#define TIMEZONE_MAP_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_TIMEZONE_MAP, CcTimezoneMapPrivate))


typedef struct
{
  gdouble offset;
  guchar red;
  guchar green;
  guchar blue;
  guchar alpha;
} CcTimezoneMapOffset;

struct _CcTimezoneMapPrivate
{
  GdkPixbuf *orig_background;
  GdkPixbuf *orig_color_map;

  GdkPixbuf *background;
  GdkPixbuf *color_map;

  guchar *visible_map_pixels;
  gint visible_map_rowstride;

  gdouble selected_offset;

  TzDB *tzdb;
  TzLocation *location;
};

enum
{
  LOCATION_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


static CcTimezoneMapOffset color_codes[] =
{
    {-11.0, 43, 0, 0, 255 },
    {-10.0, 85, 0, 0, 255 },
    {-9.5, 102, 255, 0, 255 },
    {-9.0, 128, 0, 0, 255 },
    {-8.0, 170, 0, 0, 255 },
    {-7.0, 212, 0, 0, 255 },
    {-6.0, 255, 0, 1, 255 }, // north
    {-6.0, 255, 0, 0, 255 }, // south
    {-5.0, 255, 42, 42, 255 },
    {-4.5, 192, 255, 0, 255 },
    {-4.0, 255, 85, 85, 255 },
    {-3.5, 0, 255, 0, 255 },
    {-3.0, 255, 128, 128, 255 },
    {-2.0, 255, 170, 170, 255 },
    {-1.0, 255, 213, 213, 255 },
    {0.0, 43, 17, 0, 255 },
    {1.0, 85, 34, 0, 255 },
    {2.0, 128, 51, 0, 255 },
    {3.0, 170, 68, 0, 255 },
    {3.5, 0, 255, 102, 255 },
    {4.0, 212, 85, 0, 255 },
    {4.5, 0, 204, 255, 255 },
    {5.0, 255, 102, 0, 255 },
    {5.5, 0, 102, 255, 255 },
    {5.75, 0, 238, 207, 247 },
    {6.0, 255, 127, 42, 255 },
    {6.5, 204, 0, 254, 254 },
    {7.0, 255, 153, 85, 255 },
    {8.0, 255, 179, 128, 255 },
    {9.0, 255, 204, 170, 255 },
    {9.5, 170, 0, 68, 250 },
    {10.0, 255, 230, 213, 255 },
    {10.5, 212, 124, 21, 250 },
    {11.0, 212, 170, 0, 255 },
    {11.5, 249, 25, 87, 253 },
    {12.0, 255, 204, 0, 255 },
    {12.75, 254, 74, 100, 248 },
    {13.0, 255, 85, 153, 250 },
    {-100, 0, 0, 0, 0 }
};


static void
cc_timezone_map_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cc_timezone_map_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cc_timezone_map_dispose (GObject *object)
{
  CcTimezoneMapPrivate *priv = CC_TIMEZONE_MAP (object)->priv;

  if (priv->orig_background)
    {
      g_object_unref (priv->orig_background);
      priv->orig_background = NULL;
    }

  if (priv->orig_color_map)
    {
      g_object_unref (priv->orig_color_map);
      priv->orig_color_map = NULL;
    }

  if (priv->background)
    {
      g_object_unref (priv->background);
      priv->background = NULL;
    }

  if (priv->color_map)
    {
      g_object_unref (priv->color_map);
      priv->color_map = NULL;

      priv->visible_map_pixels = NULL;
      priv->visible_map_rowstride = 0;
    }

  G_OBJECT_CLASS (cc_timezone_map_parent_class)->dispose (object);
}

static void
cc_timezone_map_finalize (GObject *object)
{
  CcTimezoneMapPrivate *priv = CC_TIMEZONE_MAP (object)->priv;

  if (priv->tzdb)
    {
      g_free (priv->tzdb);
      priv->tzdb = NULL;
    }


  G_OBJECT_CLASS (cc_timezone_map_parent_class)->finalize (object);
}

/* GtkWidget functions */
static void
cc_timezone_map_size_request (GtkWidget      *widget,
                              GtkRequisition *req)
{
  CcTimezoneMapPrivate *priv = CC_TIMEZONE_MAP (widget)->priv;

  req->width = gdk_pixbuf_get_width (priv->orig_background) / 2;
  req->height = gdk_pixbuf_get_height (priv->orig_background) / 2;

  GTK_WIDGET_CLASS (cc_timezone_map_parent_class)->size_request (widget, req);
}

static void
cc_timezone_map_size_allocate (GtkWidget     *widget,
                               GtkAllocation *allocation)
{
  CcTimezoneMapPrivate *priv = CC_TIMEZONE_MAP (widget)->priv;
  GdkPixbuf *color_map;

  priv->background = gdk_pixbuf_scale_simple (priv->orig_background,
                                              allocation->width,
                                              allocation->height,
                                              GDK_INTERP_BILINEAR);
  color_map = gdk_pixbuf_scale_simple (priv->orig_color_map,
                                       allocation->width,
                                       allocation->height,
                                       GDK_INTERP_BILINEAR);

  priv->visible_map_pixels = gdk_pixbuf_get_pixels (color_map);
  priv->visible_map_rowstride = gdk_pixbuf_get_rowstride (color_map);

  GTK_WIDGET_CLASS (cc_timezone_map_parent_class)->size_allocate (widget,
                                                                  allocation);
}

static void
cc_timezone_map_realize (GtkWidget *widget)
{
  GdkWindowAttr attr = { 0, };
  GtkAllocation allocation;
  GdkCursor *cursor;
  GdkWindow *window;

  gtk_widget_get_allocation (widget, &allocation);

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attr.window_type = GDK_WINDOW_CHILD;
  attr.wclass = GDK_INPUT_OUTPUT;
  attr.width = allocation.width;
  attr.height = allocation.height;
  attr.x = allocation.x;
  attr.y = allocation.y;
  attr.event_mask = gtk_widget_get_events (widget)
                                 | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;

  window = gdk_window_new (gtk_widget_get_parent_window (widget), &attr,
                           GDK_WA_X | GDK_WA_Y);

  gtk_widget_set_style (widget,
                        gtk_style_attach (gtk_widget_get_style (widget),
                                          window));

  gdk_window_set_user_data (window, widget);

  cursor = gdk_cursor_new (GDK_HAND2);
  gdk_window_set_cursor (window, cursor);

  gtk_widget_set_window (widget, window);
}


static gdouble
convert_longtitude_to_x (gdouble longitude, gint map_width)
{
  const gdouble xdeg_offset = -6;
  gdouble x;

  x = (map_width * (180.0 + longitude) / 360.0)
    + (map_width * xdeg_offset / 180.0);

  return x;
}

static gdouble
radians (gdouble degrees)
{
  return (degrees / 360.0) * G_PI * 2;
}

static gdouble
convert_latitude_to_y (gdouble latitude, gdouble map_height)
{
  gdouble bottom_lat = -59;
  gdouble top_lat = 81;
  gdouble top_per, y, full_range, top_offset, map_range;

  top_per = top_lat / 180.0;
  y = 1.25 * log (tan (G_PI_4 + 0.4 * radians (latitude)));
  full_range = 4.6068250867599998;
  top_offset = full_range * top_per;
  map_range = fabs (1.25 * log (tan (G_PI_4 + 0.4 * radians (bottom_lat))) - top_offset);
  y = fabs (y - top_offset);
  y = y / map_range;
  y = y * map_height;
  return y;
}


static gboolean
cc_timezone_map_expose_event (GtkWidget      *widget,
                              GdkEventExpose *event)
{
  CcTimezoneMapPrivate *priv = CC_TIMEZONE_MAP (widget)->priv;
  GdkPixbuf *hilight, *orig_hilight;
  cairo_t *cr;
  GtkAllocation alloc;
  gchar *file;
  GError *err = NULL;
  gdouble pointx, pointy;

  cr = gdk_cairo_create (gtk_widget_get_window (widget));

  gtk_widget_get_allocation (widget, &alloc);

  /* paint background */
  gdk_cairo_set_source_pixbuf (cr, priv->background, 0, 0);
  cairo_paint (cr);

  /* paint hilight */
  file = g_strdup_printf (DATADIR "/timezone_%g.png",
                          priv->selected_offset);
  orig_hilight = gdk_pixbuf_new_from_file (file, &err);
  if (!orig_hilight)
    {
      g_warning ("Could not load hilight: %s",
                 (err) ? err->message : "Unknown Error");
      if (err)
        g_clear_error (&err);
    }
  else
    {

      hilight = gdk_pixbuf_scale_simple (orig_hilight, alloc.width,
                                         alloc.height, GDK_INTERP_BILINEAR);
      gdk_cairo_set_source_pixbuf (cr, hilight, 0, 0);

      cairo_paint (cr);
      g_object_unref (hilight);
      g_object_unref (orig_hilight);
    }

  if (priv->location)
    {
      pointx = convert_longtitude_to_x (priv->location->longitude, alloc.width);
      pointy = convert_latitude_to_y (priv->location->latitude, alloc.height);

      cairo_set_source_rgb (cr, 0, 0, 0);
      cairo_arc (cr, pointx, pointy, 3.0, 0, 2 * G_PI);
      cairo_fill (cr);
    }

  cairo_destroy (cr);


  return TRUE;
}


static void
cc_timezone_map_class_init (CcTimezoneMapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CcTimezoneMapPrivate));

  object_class->get_property = cc_timezone_map_get_property;
  object_class->set_property = cc_timezone_map_set_property;
  object_class->dispose = cc_timezone_map_dispose;
  object_class->finalize = cc_timezone_map_finalize;

  widget_class->size_request = cc_timezone_map_size_request;
  widget_class->size_allocate = cc_timezone_map_size_allocate;
  widget_class->realize = cc_timezone_map_realize;
  widget_class->expose_event = cc_timezone_map_expose_event;

  signals[LOCATION_CHANGED] = g_signal_new ("location-changed",
                                            CC_TYPE_TIMEZONE_MAP,
                                            G_SIGNAL_RUN_FIRST,
                                            0,
                                            NULL,
                                            NULL,
                                            g_cclosure_marshal_VOID__POINTER,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_POINTER);
}


static gint
sort_locations (TzLocation *a,
                TzLocation *b)
{
  if (a->dist > b->dist)
    return 1;

  if (a->dist < b->dist)
    return -1;

  return 0;
}

static void
set_location (CcTimezoneMap *map,
              TzLocation    *location)
{
  CcTimezoneMapPrivate *priv = map->priv;
  TzInfo *info;

  priv->location = location;

  info = tz_info_from_location (priv->location);

  priv->selected_offset = tz_location_get_utc_offset (priv->location)
    / (60.0*60.0) + ((info->daylight) ? -1.0 : 0.0);

  g_signal_emit (map, signals[LOCATION_CHANGED], 0, priv->location);

  tz_info_free (info);
}

static gboolean
button_press_event (GtkWidget      *widget,
                    GdkEventButton *event)
{
  CcTimezoneMapPrivate *priv = CC_TIMEZONE_MAP (widget)->priv;
  gint x, y;
  guchar r, g, b, a;
  guchar *pixels;
  gint rowstride;
  gint i;

  const GPtrArray *array;
  gint width, height;
  GList *distances = NULL;
  GtkAllocation alloc;

  x = event->x;
  y = event->y;


  rowstride = priv->visible_map_rowstride;
  pixels = priv->visible_map_pixels;

  r = pixels[(rowstride * y + x * 4)];
  g = pixels[(rowstride * y + x * 4) + 1];
  b = pixels[(rowstride * y + x * 4) + 2];
  a = pixels[(rowstride * y + x * 4) + 3];


  for (i = 0; color_codes[i].offset != -100; i++)
    {
       if (color_codes[i].red == r && color_codes[i].green == g
           && color_codes[i].blue == b && color_codes[i].alpha == a)
         {
           priv->selected_offset = color_codes[i].offset;
         }
    }

  gtk_widget_queue_draw (widget);

  /* work out the co-ordinates */

  array = tz_get_locations (priv->tzdb);

  gtk_widget_get_allocation (widget, &alloc);
  width = alloc.width;
  height = alloc.height;

  for (i = 0; i < array->len; i++)
    {
      gdouble pointx, pointy, dx, dy;
      TzLocation *loc = array->pdata[i];

      pointx = convert_longtitude_to_x (loc->longitude, width);
      pointy = convert_latitude_to_y (loc->latitude, height);

      dx = pointx - x;
      dy = pointy - y;

      loc->dist = dx * dx + dy * dy;
      distances = g_list_prepend (distances, loc);

    }
  distances = g_list_sort (distances, (GCompareFunc) sort_locations);


  set_location (CC_TIMEZONE_MAP (widget), (TzLocation*) distances->data);

  g_list_free (distances);

  return TRUE;
}

static void
cc_timezone_map_init (CcTimezoneMap *self)
{
  CcTimezoneMapPrivate *priv;
  GError *err = NULL;

  priv = self->priv = TIMEZONE_MAP_PRIVATE (self);

  priv->orig_background = gdk_pixbuf_new_from_file (DATADIR "/bg.png",
                                                    &err);

  if (!priv->orig_background)
    {
      g_warning ("Could not load background image: %s",
                 (err) ? err->message : "Unknown error");
      g_clear_error (&err);
    }

  priv->orig_color_map = gdk_pixbuf_new_from_file (DATADIR "/cc.png",
                                                   &err);
  if (!priv->orig_color_map)
    {
      g_warning ("Could not load background image: %s",
                 (err) ? err->message : "Unknown error");
      g_clear_error (&err);
    }

  priv->tzdb = tz_load_db ();

  g_signal_connect (self, "button-press-event", G_CALLBACK (button_press_event),
                    NULL);

  /*
  g_signal_connect (self, "map-event", G_CALLBACK (map_event), NULL);
  g_signal_connect (self, "unmap-event", G_CALLBACK (unmap_event), NULL);
  */
}

CcTimezoneMap *
cc_timezone_map_new (void)
{
  return g_object_new (CC_TYPE_TIMEZONE_MAP, NULL);
}

void
cc_timezone_map_set_timezone (CcTimezoneMap *map,
                              const gchar   *timezone)
{
  GPtrArray *locations;
  guint i;

  locations = tz_get_locations (map->priv->tzdb);

  for (i = 0; i < locations->len; i++)
    {
      TzLocation *loc = locations->pdata[i];

      if (!g_strcmp0 (loc->zone, timezone))
        {
          set_location (map, loc);
          break;
        }
    }
}