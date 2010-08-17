/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Intel Corporation.
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

#ifndef __CC_POWER_PANEL_H
#define __CC_POWER_PANEL_H

#include <gtk/gtk.h>
#include "cc-panel.h"

G_BEGIN_DECLS

#define CC_TYPE_POWER_PANEL         (cc_power_panel_get_type ())
#define CC_POWER_PANEL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CC_TYPE_POWER_PANEL, CcPowerPanel))
#define CC_POWER_PANEL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), CC_TYPE_POWER_PANEL, CcPowerPanelClass))
#define CC_IS_POWER_PANEL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CC_TYPE_POWER_PANEL))
#define CC_IS_POWER_PANEL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CC_TYPE_POWER_PANEL))
#define CC_POWER_PANEL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CC_TYPE_POWER_PANEL, CcPowerPanelClass))

typedef struct CcPowerPanelPrivate CcPowerPanelPrivate;

typedef struct
{
        CcPanel              parent;
        CcPowerPanelPrivate *priv;
} CcPowerPanel;

typedef struct
{
        CcPanelClass   parent_class;
} CcPowerPanelClass;

GType              cc_power_panel_get_type   (void);
void               cc_power_panel_register   (GIOModule         *module);

G_END_DECLS

#endif /* __CC_POWER_PANEL_H */
