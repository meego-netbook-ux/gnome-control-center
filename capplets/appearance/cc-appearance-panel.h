/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __CC_APPEARANCE_PANEL_H
#define __CC_APPEARANCE_PANEL_H

#include <gtk/gtk.h>
#include "cc-panel.h"

G_BEGIN_DECLS

#define CC_TYPE_APPEARANCE_PANEL         (cc_appearance_panel_get_type ())
#define CC_APPEARANCE_PANEL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CC_TYPE_APPEARANCE_PANEL, CcAppearancePanel))
#define CC_APPEARANCE_PANEL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), CC_TYPE_APPEARANCE_PANEL, CcAppearancePanelClass))
#define CC_IS_APPEARANCE_PANEL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CC_TYPE_APPEARANCE_PANEL))
#define CC_IS_APPEARANCE_PANEL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CC_TYPE_APPEARANCE_PANEL))
#define CC_APPEARANCE_PANEL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CC_TYPE_APPEARANCE_PANEL, CcAppearancePanelClass))

typedef struct CcAppearancePanelPrivate CcAppearancePanelPrivate;

typedef struct
{
        CcPanel                 parent;
        CcAppearancePanelPrivate *priv;
} CcAppearancePanel;

typedef struct
{
        CcPanelClass   parent_class;
} CcAppearancePanelClass;

GType              cc_appearance_panel_get_type   (void);
void               cc_appearance_panel_register   (GIOModule         *module);

G_END_DECLS

#endif /* __CC_APPEARANCE_PANEL_H */