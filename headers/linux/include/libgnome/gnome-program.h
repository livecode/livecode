/*
 * Copyright (C) 1999, 2000 Red Hat, Inc.
 *               2001 SuSE Linux AG.
 * All rights reserved.
 *
 * This file is part of GNOME 2.0.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */


/* This module takes care of handling application and library
   initialization and command line parsing */

#ifndef GNOME_PROGRAM_H
#define GNOME_PROGRAM_H

#include <glib.h>
#include <stdarg.h>
#include <errno.h>

#include <glib-object.h>

#ifndef GNOME_DISABLE_DEPRECATED
#include <popt.h>
#endif

G_BEGIN_DECLS

#define GNOME_TYPE_PROGRAM            (gnome_program_get_type ())
#define GNOME_PROGRAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_PROGRAM, GnomeProgram))
#define GNOME_PROGRAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_PROGRAM, GnomeProgramClass))
#define GNOME_IS_PROGRAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_PROGRAM))
#define GNOME_IS_PROGRAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_PROGRAM))

typedef struct _GnomeProgram          GnomeProgram;
typedef struct _GnomeProgramPrivate   GnomeProgramPrivate;
typedef struct _GnomeProgramClass     GnomeProgramClass;

typedef enum {
    GNOME_FILE_DOMAIN_UNKNOWN = 0,

    /* Gnome installed files */
    GNOME_FILE_DOMAIN_LIBDIR,
    GNOME_FILE_DOMAIN_DATADIR,
    GNOME_FILE_DOMAIN_SOUND,
    GNOME_FILE_DOMAIN_PIXMAP,
    GNOME_FILE_DOMAIN_CONFIG,
    GNOME_FILE_DOMAIN_HELP,

    /* Application files */
    GNOME_FILE_DOMAIN_APP_LIBDIR,
    GNOME_FILE_DOMAIN_APP_DATADIR,
    GNOME_FILE_DOMAIN_APP_SOUND,
    GNOME_FILE_DOMAIN_APP_PIXMAP,
    GNOME_FILE_DOMAIN_APP_CONFIG,
    GNOME_FILE_DOMAIN_APP_HELP
} GnomeFileDomain;

struct _GnomeProgram
{
    GObject object;

    GnomeProgramPrivate *_priv;
};

struct _GnomeProgramClass
{
    GObjectClass object_class;

    /* we may want to add stuff in the future */
    gpointer padding1;
    gpointer padding2;
};

GType
gnome_program_get_type                  (void);

GnomeProgram *
gnome_program_get                       (void);

const char *
gnome_program_get_human_readable_name   (GnomeProgram *program);

const char *
gnome_program_get_app_id                (GnomeProgram *program);

const char *
gnome_program_get_app_version           (GnomeProgram *program);

gchar *
gnome_program_locate_file               (GnomeProgram    *program,
					 GnomeFileDomain  domain,
					 const gchar     *file_name,
					 gboolean         only_if_exists,
					 GSList         **ret_locations);

#define GNOME_PARAM_NONE                NULL
#define GNOME_PARAM_GOPTION_CONTEXT     "goption-context"
#define GNOME_PARAM_CREATE_DIRECTORIES  "create-directories"
#define GNOME_PARAM_ENABLE_SOUND        "enable-sound"
#define GNOME_PARAM_ESPEAKER            "espeaker"
#define GNOME_PARAM_APP_ID              "app-id"
#define GNOME_PARAM_APP_VERSION         "app-version"
#define GNOME_PARAM_GNOME_PREFIX        "gnome-prefix"
#define GNOME_PARAM_GNOME_SYSCONFDIR    "gnome-sysconfdir"
#define GNOME_PARAM_GNOME_DATADIR       "gnome-datadir"
#define GNOME_PARAM_GNOME_LIBDIR        "gnome-libdir"
#define GNOME_PARAM_APP_PREFIX          "app-prefix"
#define GNOME_PARAM_APP_SYSCONFDIR      "app-sysconfdir"
#define GNOME_PARAM_APP_DATADIR         "app-datadir"
#define GNOME_PARAM_APP_LIBDIR          "app-libdir"
#define GNOME_PARAM_HUMAN_READABLE_NAME "human-readable-name"
#define GNOME_PARAM_GNOME_PATH          "gnome-path"

#ifndef GNOME_DISABLE_DEPRECATED
#define GNOME_PARAM_POPT_TABLE          "popt-table"
#define GNOME_PARAM_POPT_FLAGS          "popt-flags"
#define GNOME_PARAM_POPT_CONTEXT        "popt-context"
#endif

/***** application modules (aka libraries :) ******/
#define GNOME_TYPE_MODULE_INFO          (gnome_module_info_get_type ())

GType
gnome_module_info_get_type              (void);

typedef struct _GnomeModuleInfo GnomeModuleInfo;
typedef struct _GnomeModuleRequirement GnomeModuleRequirement;

struct _GnomeModuleRequirement {
    const char *required_version;
    const GnomeModuleInfo *module_info;
};

typedef void (*GnomeModuleInitHook) (const GnomeModuleInfo *mod_info);
typedef void (*GnomeModuleClassInitHook) (GnomeProgramClass *klass,
					  const GnomeModuleInfo *mod_info);
typedef void (*GnomeModuleHook) (GnomeProgram *program,
				 GnomeModuleInfo *mod_info);
typedef GOptionGroup* (*GnomeModuleGetGOptionGroupFunc) (void);

struct _GnomeModuleInfo {
    const char *name;
    const char *version;
    const char *description;
    GnomeModuleRequirement *requirements; /* last element has NULL version */

    GnomeModuleHook instance_init;
    GnomeModuleHook pre_args_parse, post_args_parse;

#ifdef GNOME_DISABLE_DEPRECATED
    void *_options;
#else
    struct poptOption *options;
#endif

    GnomeModuleInitHook init_pass;

    GnomeModuleClassInitHook class_init;

    const char *opt_prefix;
    GnomeModuleGetGOptionGroupFunc get_goption_group_func;
};

/* This function should be called before gnomelib_preinit() - it's an
 * alternative to the "module" property passed by the app.
 */
void
gnome_program_module_register (const GnomeModuleInfo *module_info);

gboolean
gnome_program_module_registered (const GnomeModuleInfo *module_info);

const GnomeModuleInfo *
gnome_program_module_load (const char *mod_name);

guint
gnome_program_install_property (GnomeProgramClass *pclass,
				GObjectGetPropertyFunc get_fn,
				GObjectSetPropertyFunc set_fn,
				GParamSpec *pspec);

#ifndef GNOME_DISABLE_DEPRECATED

/*
 * If the application writer wishes to use getopt()-style arg
 * processing, they can do it using a while looped sandwiched between
 * calls to these two functions.
 */
poptContext
gnome_program_preinit (GnomeProgram *program,
		       const char *app_id,
		       const char *app_version,
		       int argc, char **argv);

void
gnome_program_parse_args (GnomeProgram *program);

void
gnome_program_postinit (GnomeProgram *program);

#endif /* GNOME_DISABLE_DEPRECATED */

/* If you have your auto* define PREFIX, SYSCONFDIR, DATADIR and LIBDIR,
 * Use this macro in your init code. */
#define GNOME_PROGRAM_STANDARD_PROPERTIES \
	GNOME_PARAM_APP_PREFIX, PREFIX,		\
	GNOME_PARAM_APP_SYSCONFDIR, SYSCONFDIR,	\
	GNOME_PARAM_APP_DATADIR, DATADIR,	\
	GNOME_PARAM_APP_LIBDIR, LIBDIR

GnomeProgram *
gnome_program_init (const char *app_id, const char *app_version,
		    const GnomeModuleInfo *module_info,
		    int argc, char **argv,
		    const char *first_property_name, ...);

GnomeProgram *
gnome_program_initv (GType type,
		     const char *app_id, const char *app_version,
		     const GnomeModuleInfo *module_info,
		     int argc, char **argv,
		     const char *first_property_name, va_list args);
GnomeProgram*
gnome_program_init_paramv (GType type,
                           const char *app_id, const char *app_version,
                           const GnomeModuleInfo *module_info,
                           int argc, char **argv,
                           guint nparams, GParameter *params);

G_END_DECLS

#endif /* GNOME_PROGRAM_H */
