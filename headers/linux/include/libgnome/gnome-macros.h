/* gnome-macros.h
 *   Macros for making GObject objects to avoid typos and reduce code size
 * Copyright (C) 2000  Eazel, Inc.
 *
 * Authors: George Lebl <jirka@5z.com>
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
*/

#ifndef GNOME_MACROS_H
#define GNOME_MACROS_H

#ifndef GNOME_DISABLE_DEPRECATED

#include <bonobo/bonobo-macros.h>

/* Macros for defining classes.  Ideas taken from Nautilus and GOB. */

/* Define the boilerplate type stuff to reduce typos and code size.  Defines
 * the get_type method and the parent_class static variable. */
#define GNOME_CLASS_BOILERPLATE(type, type_as_function,			\
				parent_type, parent_type_macro)		\
	BONOBO_BOILERPLATE(type, type_as_function, type,		\
			  parent_type, parent_type_macro,		\
			  GNOME_REGISTER_TYPE)
#define GNOME_REGISTER_TYPE(type, type_as_function, corba_type,		\
			    parent_type, parent_type_macro)		\
	g_type_register_static (parent_type_macro, #type, &object_info, 0)

/* Just call the parent handler.  This assumes that there is a variable
 * named parent_class that points to the (duh!) parent class.  Note that
 * this macro is not to be used with things that return something, use
 * the _WITH_DEFAULT version for that */
#define GNOME_CALL_PARENT(parent_class_cast, name, args)		\
	BONOBO_CALL_PARENT (parent_class_cast, name, args)

/* Same as above, but in case there is no implementation, it evaluates
 * to def_return */
#define GNOME_CALL_PARENT_WITH_DEFAULT(parent_class_cast,		\
				       name, args, def_return)		\
	BONOBO_CALL_PARENT_WITH_DEFAULT (				\
		parent_class_cast, name, args, def_return)

#endif /* !GNOME_DISABLE_DEPRECATED */

#endif /* GNOME_MACROS_H */
