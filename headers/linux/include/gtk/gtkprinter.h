/* GtkPrinter 
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GTK_PRINTER_H__
#define __GTK_PRINTER_H__

#include <glib-object.h>
#include <cairo.h>
#include <gtk/gtkprintsettings.h>
#include <gtk/gtkpagesetup.h>

G_BEGIN_DECLS

#define GTK_TYPE_PRINTER                  (gtk_printer_get_type ())
#define GTK_PRINTER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_PRINTER, GtkPrinter))
#define GTK_PRINTER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_PRINTER, GtkPrinterClass))
#define GTK_IS_PRINTER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_PRINTER))
#define GTK_IS_PRINTER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_PRINTER))
#define GTK_PRINTER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_PRINTER, GtkPrinterClass))

typedef struct _GtkPrinter          GtkPrinter;
typedef struct _GtkPrinterClass     GtkPrinterClass;
typedef struct _GtkPrinterPrivate   GtkPrinterPrivate;
typedef struct _GtkPrintBackend     GtkPrintBackend;

struct _GtkPrintBackend;

struct _GtkPrinter
{
  GObject parent_instance;

  GtkPrinterPrivate *priv;
};

struct _GtkPrinterClass
{
  GObjectClass parent_class;

  void (*details_acquired) (GtkPrinter *printer, gboolean success);
  
  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
  void (*_gtk_reserved5) (void);
  void (*_gtk_reserved6) (void);
  void (*_gtk_reserved7) (void);
};

GType                    gtk_printer_get_type          (void) G_GNUC_CONST;
GtkPrinter              *gtk_printer_new               (const gchar     *name,
							GtkPrintBackend *backend,
							gboolean         virtual_);
GtkPrintBackend         *gtk_printer_get_backend       (GtkPrinter      *printer);
G_CONST_RETURN gchar    *gtk_printer_get_name          (GtkPrinter      *printer);
G_CONST_RETURN gchar    *gtk_printer_get_state_message (GtkPrinter      *printer);
G_CONST_RETURN gchar    *gtk_printer_get_description   (GtkPrinter      *printer);
G_CONST_RETURN gchar    *gtk_printer_get_location      (GtkPrinter      *printer);
G_CONST_RETURN gchar    *gtk_printer_get_icon_name     (GtkPrinter      *printer);
gint                     gtk_printer_get_job_count     (GtkPrinter      *printer);
gboolean                 gtk_printer_is_active         (GtkPrinter      *printer);
gboolean                 gtk_printer_is_virtual        (GtkPrinter      *printer);
gboolean                 gtk_printer_is_default        (GtkPrinter      *printer);
gboolean                 gtk_printer_accepts_pdf       (GtkPrinter      *printer);
gboolean                 gtk_printer_accepts_ps        (GtkPrinter      *printer);

gint                     gtk_printer_compare           (GtkPrinter *a,
							GtkPrinter *b);

typedef gboolean (*GtkPrinterFunc) (GtkPrinter *printer,
				    gpointer    data);

void                     gtk_enumerate_printers        (GtkPrinterFunc   func,
							gpointer         data,
							GDestroyNotify   destroy,
							gboolean         wait);
						      
G_END_DECLS

#endif /* __GTK_PRINTER_H__ */
