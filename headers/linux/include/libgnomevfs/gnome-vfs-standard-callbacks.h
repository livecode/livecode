/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*

   Copyright (C) 2001 Eazel, Inc
   Copyright (C) 2002 Seth Nickell

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Michael Fleming <mfleming@eazel.com>
           Seth Nickell <snickell@stanford.edu>
*/

#ifndef GNOME_VFS_STANDARD_CALLBACKS_H
#define GNOME_VFS_STANDARD_CALLBACKS_H

// MDW 2013-06-05 : error: #error "Only <glib.h> can be included directly."
//#include <glib/gtypes.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs-uri.h>

G_BEGIN_DECLS

/*
 * defined callback structures
 */


/**
 * GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION:
 *
 * A module callback name used together with gnome_vfs_module_callback_invoke() to
 * ask the user for login data. This includes username and password, but also
 * special login choices like anonymous login.
 *
 * <variablelist role="params">
 * <varlistentry><term><parameter>in arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackFullAuthenticationIn * passed to the application,
 * specifying the authentication request.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * <varlistentry><term><parameter>out arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackFullAuthenticationOut * passing the user's
 * provided authentication data (i.e. his username/password etc.) back to the module.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * </variablelist>
 */
#define GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION "full-authentication"

/**
 * GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION:
 *
 * A module callback name used together with gnome_vfs_module_callback_invoke() to
 * ask the keyring manager for login data. It is expected to return stored or cached
 * login data, but may not query the user.
 *
 * The login data consists of a username, a password and a domain, and is used to
 * access a resource. If the data is not suitable for accessing the resource, the
 * #GnomeVFSModule typically issues a #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION
 * module callback to query the user right after the authentication failed.
 *
 * <variablelist role="params">
 * <varlistentry><term><parameter>in arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackFillAuthenticationIn * passed to the application,
 * specifying the authentication request. The application will usually proxy this
 * request to the keyring manager of the desktop environment, but it can also handle
 * the request itself.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * <varlistentry><term><parameter>out arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackFillAuthenticationOut * passing the cached
 * keyring data (i.e. username/password etc.) back to the module. Usually,
 * this is data that was stored using #GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
 * and is either stored permanently on disk or cached.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * </variablelist>
 **/
#define GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION "fill-authentication"

/**
 * GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION:
 *
 * A module callback name used together with gnome_vfs_module_callback_invoke() to
 * request permanent or temporary storage of login data. The storage is typically
 * done using a keyring manager.
 * 
 * Called after a successfull authentication, to allow the client to e.g.
 * store the password for future use. It may be queried again within the
 * current session (temporary storage) or in future sessions (permanent
 * storage) using #GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION.
 *
 * This is typically called after the user provided login data with
 * #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION.
 *
 * <variablelist role="params">
 * <varlistentry><term><parameter>in arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackSaveAuthenticationIn * specifying the login data.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * <varlistentry><term><parameter>out arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackSaveAuthenticationOut * (unused).
 * </simpara>
 * </listitem>
 * </varlistentry>
 * </variablelist>
 */
#define GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION "save-authentication"

#ifndef GNOME_VFS_DISABLE_DEPRECATED

/**
 * GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION
 *
 * A module callback name formerly used together with gnome_vfs_module_callback_invoke().
 * 
 * Formerly called when access to a URI requires a username/password.
 *
 * <variablelist role="params">
 * <varlistentry><term><parameter>in arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackAuthenticationIn *.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * <varlistentry><term><parameter>out arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackAuthenticationOut *.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * </variablelist>
 *
 * Deprecated: Modules should use #GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION
 * 	       and #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION.
 */
#define GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION "simple-authentication"

/**
 * GNOME_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION:
 *
 * A module callback name formerly used together with gnome_vfs_module_callback_invoke()
 * to ask the user for HTTP proxy login data. It works exactly like
 * #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION, and used to be kept separate to
 * allow applications to distinguish proxy authentication from actual server
 * authentication, so that the wording of the interface could be adapted.
 * 
 * Formerly called when access to an HTTP proxy required a username/password.
 *
 * <variablelist role="params">
 * <varlistentry><term><parameter>in arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackAuthenticationIn *.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * <varlistentry><term><parameter>out arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackAuthenticationOut *.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * </variablelist>
 *
 * Deprecated: Proxy authentication now works desktop-wide through GConf.
 **/
#define GNOME_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION "http:proxy-authentication"

#endif /* GNOME_VFS_DISABLE_DEPRECATED */

/**
 * GNOME_VFS_MODULE_CALLBACK_QUESTION:
 *
 * A module callback name used together with gnome_vfs_module_callback_invoke() to
 * ask the user a question.
 * 
 * Called when access to a URI requires the user to make a choice.
 *
 * <variablelist role="params">
 * <varlistentry><term><parameter>in arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackFullAuthenticationIn * passed to the application,
 * </simpara>
 * </listitem>
 * </varlistentry>
 * <varlistentry><term><parameter>out arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * specifying the proxy authentication request.
 * A #GnomeVFSModuleCallbackFullAuthenticationOut * passing the user's
 * provided proxy authentication data (i.e. his username/password) back to the module.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * </variablelist>
 */
#define GNOME_VFS_MODULE_CALLBACK_QUESTION "ask-question"

/**
 * GnomeVFSModuleCallbackFillAuthenticationIn:
 * @uri: The textual URI of the resource that requires authentication.
 * @protocol: One of the protocols supported by the invoking module.
 * Typically matches @uri's protocol.
 * @server: The server that contains the resource that requires authentication.
 * Typically matches @uri's hostname.
 * @object: The type of the resource that requires authentication.
 * @port: The port that was used to connect to @server. 0 means unset.
 * @authtype: The type of authentication that was requested. For the
 * HTTP method, this may be "basic" or "proxy". For the SFTP method,
 * this may be "publickey" or "password".
 * @username: The username that was used to connect to @server.
 * @domain: The domain that @server belongs to (only used by the SMB method).
 *
 * A pointer to a #GnomeVFSModuleCallbackFillAuthenticationIn structure is passed to the
 * #GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION callback, and informs the application
 * about the authentication parameters that should be requested from the user.
 *
 * The #GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION application callback will then set
 * the members of a pointer to a #GnomeVFSModuleCallbackFillAuthenticationOut structure
 * according to the stored or cached data.
 **/
typedef struct {
	/*< public >*/
	char *uri;
	char *protocol;
	char *server;
	char *object;
	int port;
	char *authtype;
	char *username;
	char *domain;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackFillAuthenticationIn;

/**
 * GnomeVFSModuleCallbackFillAuthenticationOut:
 * @valid: Whether stored or cached login data was found for the resource referenced by
 * the #GnomeVFSModuleCallbackFillAuthenticationIn structure.
 * @username: The cached username that should be used to access the resource.
 * This will be freed by the module when it isn't needed any longer. May only
 * be set if @valid is %TRUE.
 * @password: The cached password that should be used to access the resource.
 * This will be freed by the module when it isn't needed any longer. May only
 * be set if @valid is %TRUE.
 * @domain: The cached domain that should be used to access the resource.
 * This will be freed by the module when it isn't needed any longer. May only
 * be set if @valid is %TRUE.
 *
 * A pointer to a #GnomeVFSModuleCallbackFillAuthenticationOut structure is passed to the
 * #GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION callback, and informs the module
 * about the authentication parameters that were found in the cache or permanently stored.
 *
 * The login data returned by #GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION was usually
 * previously stored using ##GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION.
 **/
typedef struct {
	/*< public >*/
	gboolean valid;
	char *username;
	char *domain;
	char *password;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackFillAuthenticationOut;

/**
 * GnomeVFSModuleCallbackFullAuthenticationFlags:
 * @GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_PREVIOUS_ATTEMPT_FAILED: This is not the first login attempt,
 * i.e. this callback was already invoked but the provided login data was not suitable for a successful login.
 * @GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_PASSWORD: The application should ask the user for
 * a password and set the @password field of #GnomeVFSModuleCallbackFullAuthenticationOut.
 * @GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME: The application should ask the user for
 * a username and set the @username field of #GnomeVFSModuleCallbackFullAuthenticationOut.
 * @GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_DOMAIN: The application should ask the user for
 * a domain and set the @domain field of #GnomeVFSModuleCallbackFullAuthenticationOut.
 * @GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_SAVING_SUPPORTED: The application may ask the user
 * whether he wants to save the password. If this flag is not present, or the user does not want to
 * save the password, the application must set the @save_password field of #GnomeVFSModuleCallbackFullAuthenticationOut
 * to %FALSE and its keyring field to %NULL. If the @save_password field is %TRUE, the module invoking
 * the #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION callback is expected to invoke the
 * #GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION callback if the login attempt was successful with
 * the @username, @password, @domain and @keyring fields of #GnomeVFSModuleCallbackFullAuthenticationOut.
 * @GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_ANON_SUPPORTED: The application
 * should offer the user a choice to login anonymously (used for example by the FTP module).
 * If the user requests anonymous login, #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_OUT_ANON_SELECTED
 * must be set in the @out_flags field of #GnomeVFSModuleCallbackFullAuthenticationOut.
 *
 * These flags will be passed to the #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION callback,
 * as part of the #GnomeVFSModuleCallbackFullAuthenticationIn structure. The output data
 * will be stored in a #GnomeVFSModuleCallbackFullAuthenticationOut structure.
 **/
typedef enum {
	GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_PREVIOUS_ATTEMPT_FAILED = 1<<0,
	GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_PASSWORD = 1<<1,
	GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME = 1<<2,
	GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_DOMAIN = 1<<3,
	GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_SAVING_SUPPORTED = 1<<4,
	GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_ANON_SUPPORTED = 1<<5
} GnomeVFSModuleCallbackFullAuthenticationFlags;

/**
 * GnomeVFSModuleCallbackFullAuthenticationIn:
 * @flags: #GnomeVFSModuleCallbackFullAuthenticationFlags influencing the user query.
 * @uri: The textual URI of the resource that requires authentication.
 * @protocol: One of the protocols supported by the invoking module. Typically matches @uri's protocol.
 * @server: The server that contains the resource that requires authentication. Typically matches @uri's hostname.
 * @object: The type of the resource that requires authentication.
 * @port: The port that was used to connect to @server. 0 means unset.
 * @authtype: The type of authentication that was requested. For the
 * HTTP method, this may be "basic" or "proxy". For the SFTP method,
 * this may be "publickey" or "password".
 * @username: The username that was used to connect to @server. This variable
 * should just be used for display purposes, i.e. like <quote>You were trying
 * to access foo@bar.com</quote> where foo is the @username and bar.com is the
 * @server. Do not make the contents of the #GnomeVFSModuleCallbackFullAuthenticationOut
 * output variables depend on the value of this variable. If you want to handle
 * absence of user input, pass back @default_user instead.
 * @domain: The domain that @server belongs to (only used by the SMB method).
 * This variable should just be used for display purposes where foo is the @username
 * and bar.com is the @server. Do not make the contents of the
 * #GnomeVFSModuleCallbackFullAuthenticationOut output variables depend on the value
 * of this variable. If you want to handle absence of user input, pass back
 * @default_domain instead.
 * @default_user: The username that should be provided to the user by default.
 * Typically matches @username.
 * @default_domain: The domain that should be provided to the user by default.
 * Typically matches @domain.
 *
 * A pointer to a #GnomeVFSModuleCallbackFullAuthenticationIn structure is passed to the
 * #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION callback, and informs the application
 * about the authentication parameters that should be requested from the user.
 *
 * The #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION application callback will then set
 * the members of a pointer to a #GnomeVFSModuleCallbackFullAuthenticationOut structure
 * according to the user input.
 **/
typedef struct {
	/*< public >*/
	GnomeVFSModuleCallbackFullAuthenticationFlags flags;

	char *uri;
	char *protocol;
	char *server;
	char *object;
	int port;
	char *authtype;
	char *username;
	char *domain;

	/* for pre-filling the dialog */
	char *default_user;     
	char *default_domain;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackFullAuthenticationIn;

/**
 * GnomeVFSModuleCallbackFullAuthenticationOutFlags:
 * @GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_OUT_ANON_SELECTED: Anonymous login requested.
 * May only be set if the #GnomeVFSModuleCallbackFullAuthenticationIn's
 * #GnomeVFSModuleCallbackFullAuthenticationFlags contain
 * #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_ANON_SUPPORTED.
 *
 * These flags will be passed from a #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION callback
 * back to the module, as part of the #GnomeVFSModuleCallbackFullAuthenticationOut structure.
 **/
typedef enum {
	GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_OUT_ANON_SELECTED = 1<<0
} GnomeVFSModuleCallbackFullAuthenticationOutFlags;

/**
 * GnomeVFSModuleCallbackFullAuthenticationOut:
 * @abort_auth: Whether the user somehow cancelled the login process. The application
 * is expected to offer the user a cancellation point during the authentication query.
 * In a graphical user interface, this is typically achieved by providing a "Cancel"
 * button.
 * @username: The user-provided username that should be used to access the resource
 * referenced by #GnomeVFSModuleCallbackFullAuthenticationIn. This will be freed by
 * the module when it isn't needed any longer. Must not be set if the
 * #GnomeVFSModuleCallbackFullAuthenticationFlags don't contain
 * #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME.
 * @domain: The user-provided domain that should be used to access the resource
 * referenced by #GnomeVFSModuleCallbackFullAuthenticationIn. This will be freed by
 * the module when it isn't needed any longer. Must not be set if the
 * #GnomeVFSModuleCallbackFullAuthenticationFlags don't contain
 * #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_DOMAIN.
 * @password: The user-provided password that should be used to access the resource
 * referenced by #GnomeVFSModuleCallbackFullAuthenticationIn. This will be freed by
 * the module when it isn't needed any longer. Must not be set if the
 * #GnomeVFSModuleCallbackFullAuthenticationFlags don't contain
 * #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_PASSOWRD.
 * @save_password: Flags whether the user requested to save the provided login
 * data. Must be %FALSE if the #GnomeVFSModuleCallbackFullAuthenticationFlags
 * don't contain #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_SAVING_SUPPORTED.
 * @keyring: Flags which keyring should be used to save the password. This will
 * later be passed to the #GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION callback
 * if the login attempt was successful with the specified @username, @password and
 * @domain. This will NOT be freed by the module, so the application typically
 * provides a %NULL pointer or a pointer to a static string.
 * @out_flags: #GnomeVFSModuleCallbackFullAuthenticationOutFlags signalling a special request, for instance
 * anonymous access to an FTP server.
 *
 * A pointer to a #GnomeVFSModuleCallbackFullAuthenticationOut structure is passed to the
 * #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION callback, and informs the module
 * about the authentication parameters that the user provided by setting the respective
 * fields according to the user input and the passed-in #GnomeVFSModuleCallbackFullAuthenticationIn
 * pointer.
 **/
typedef struct {
	/*< public >*/
	gboolean abort_auth;

	char *username;
	char *domain;
	char *password;

	gboolean save_password;
	char *keyring;

	gsize out_flags;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved2;
} GnomeVFSModuleCallbackFullAuthenticationOut;

/**
 * GnomeVFSModuleCallbackSaveAuthenticationIn:
 * @keyring: Keyring provided by #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION callback.
 * @uri: The textual URI of the resource that is accessed.
 * @protocol: One of the protocols supported by the invoking module.
 * Typically matches @uri's protocol.
 * @server: The server that contains the resource that is accessed.
 * Typically matches @uri's hostname.
 * @object: The type of the resource that is accessed.
 * @port: The port that was used to connect to @server. 0 means unset.
 * @authtype: The type of authentication that was requested. For the
 * HTTP method, this may be "basic" or "proxy". For the SFTP method,
 * this may be "publickey" or "password".
 * @username: The username that was used to connect to @server.
 * @domain: The domain that @server belongs to (only used by the SMB method).
 * @password: The password that was used to connect to @server.
 *
 * A #GnomeVFSModuleCallbackSaveAuthenticatioIn pointer is passed to a
 * #GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION application callback,
 * and specifies the login data that should be stored permanently on
 * disk or temporarily cached.
 *
 * This data may be queried in future sessions (permanent storage) or within
 * the current session (temporary storage) using
 * #GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION.
 **/
typedef struct {
	/*< public >*/
	char *keyring;
	
	char *uri;
	char *protocol;
	char *server;
	char *object;
	int port;
	char *authtype;
	char *username;
	char *domain;
	char *password;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackSaveAuthenticationIn;

/**
 * GnomeVFSModuleCallbackSaveAuthenticationOut:
 *
 * A #GnomeVFSModuleCallbackSaveAuthenticationOut pointer is passed to a
 * #GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION application callback,
 * and is reserved for future use. Applications are not expected
 * to modify this data, because its interpretation might change
 * in the future.
 **/
typedef struct {
	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackSaveAuthenticationOut;

#ifndef GNOME_VFS_DISABLE_DEPRECATED


/**
 * GnomeVFSModuleCallbackAuthenticationAuthType:
 * @AuthTypeBasic: transmit password unencrypted.
 * @AuthTypeDigest: transmit digest instead of plaintext credentials.
 *
 * This defines the possible values of the #GnomeVFSModuleCallbackAuthenticationIn's
 * %auth_type field.
 *
 * Deprecated: Modules should use #GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION
 * 	       and #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION.
 **/
typedef enum {
	AuthTypeBasic,
	AuthTypeDigest
} GnomeVFSModuleCallbackAuthenticationAuthType;

/**
 * GnomeVFSModuleCallbackAuthenticationIn:
 * @uri: The textual URI of the resource that requires authentication.
 * @realm: "auth" for HTTP, %NULL for others.
 * @previous_attempt_failed: %TRUE if there already was login data
 * specified for this request, but the login failed. %FALSE if no
 * previous login attempt has been made right before this one.
 * @auth_type: Whether the login data will be transmitted in plaintext or encrypted.
 *
 * A pointer to a #GnomeVFSModuleCallbackAuthenticationIn structure that used to be
 * passed to the #GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION callback, and informed
 * the application about the authentication parameters that should be requested.
 *
 * Deprecated: Modules should use #GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION
 * 	       and #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION.
 **/
typedef struct {
	/*< public >*/
	char *uri;
	char *realm;
	gboolean previous_attempt_failed;
	GnomeVFSModuleCallbackAuthenticationAuthType auth_type;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackAuthenticationIn;

#endif /* GNOME_VFS_DISABLE_DEPRECATED */

#ifndef GNOME_VFS_DISABLE_DEPRECATED

/**
 * GnomeVFSModuleCallbackAuthenticationOut:
 * @username: The username that should be used to access the resource
 * referenced by #GnomeVFSModuleCallbackFullAuthenticationIn, or %NULL
 * if no data was provided. This will be freed by the module when it
 * isn't needed any longer.
 * @password: The password that should be used to access the resource
 * referenced by #GnomeVFSModuleCallbackFullAuthenticationIn. This will
 * be freed by the module when it isn't needed any longer.
 *
 * A pointer to a #GnomeVFSModuleCallbackAuthenticationOut structure that used to be
 * passed to the #GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION callback, and was
 * used to pass the login data back to the module.
 *
 * Deprecated: Modules should use #GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION
 * 	       and #GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION.
 **/
typedef struct {
	/*< public >*/
	char *username;
	char *password;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackAuthenticationOut;

#endif /* GNOME_VFS_DISABLE_DEPRECATED */

/**
 * GNOME_VFS_MODULE_CALLBACK_HTTP_SEND_ADDITIONAL_HEADERS:
 *
 * A module callback name used together with gnome_vfs_module_callback_invoke() to
 * request additional HTTP headers.
 *
 * Called before sending headers to an HTTP server. Client applications can add
 * additional headers.
 *
 * <variablelist role="params">
 * <varlistentry><term><parameter>in arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackAdditionalHeadersIn * identifying the resource.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * <varlistentry><term><parameter>out arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackAdditionalHeadersOut * allowing to add headers
 * to the request.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * </variablelist>
 **/
#define GNOME_VFS_MODULE_CALLBACK_HTTP_SEND_ADDITIONAL_HEADERS "http:send-additional-headers"

/**
 * GnomeVFSModuleCallbackAdditionalHeadersIn:
 * @uri: A #GnomeVFSURI identifying the resource of the currently pending request.
 *
 * A #GnomeVFSModuleCallbackAdditionalHeadersIn pointer is passed to a
 * #GNOME_VFS_MODULE_CALLBACK_HTTP_SEND_ADDITIONAL_HEADERS application
 * callback, to inform it about a pending HTTP request and allow it to
 * add additional headers.
 **/
typedef struct {
	/*< public >*/
	GnomeVFSURI *uri;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackAdditionalHeadersIn;

/**
 * GnomeVFSModuleCallbackAdditionalHeadersOut:
 * @headers: A #GList of strings, each of them is an additional header that
 * is added to the HTTP request. @headers and all its strings will be freed
 * by the module when they aren't needed any longer.
 *
 * A #GnomeVFSModuleCallbackAdditionalHeadersOut pointer is passed to a
 * #GNOME_VFS_MODULE_CALLBACK_HTTP_SEND_ADDITIONAL_HEADERS application
 * callback, to store the headers the application wants to add to
 * a pending HTTP request.
 **/
typedef struct {
	/*< public >*/
	GList *headers;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackAdditionalHeadersOut;

/**
 * GNOME_VFS_MODULE_CALLBACK_HTTP_RECEIVED_HEADERS:
 *
 * A module callback name used together with gnome_vfs_module_callback_invoke() to
 * inform an application about the delivery of a HTTP request.
 *
 * #GNOME_VFS_MODULE_CALLBACK_HTTP_RECEIVED_HEADERS is called after
 * receiving HTTP headers from a server that belonged to a HTTP
 * request that was issued by the application and allows the
 * application to analyze the returned headers.
 *
 * <variablelist role="params">
 * <varlistentry><term><parameter>in arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackReceivedHeadersIn *.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * <varlistentry><term><parameter>out arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackReceivedHeadersOut * (not used).
 * </simpara>
 * </listitem>
 * </varlistentry>
 * </variablelist>
 */
#define GNOME_VFS_MODULE_CALLBACK_HTTP_RECEIVED_HEADERS "http:received-headers"

/**
 * GnomeVFSModuleCallbackReceivedHeadersIn:
 * @uri: A #GnomeVFSURI identifying the resource of the currently delivered request.
 * @headers: A #GList of strings, each of them is a header that was received when
 * delivering the HTTP request. @headers and all its strings will be freed
 * by the module when they aren't needed any longer.
 *
 * A #GnomeVFSModuleCallbackReceivedHeadersIn pointer is passed to a
 * #GNOME_VFS_MODULE_CALLBACK_HTTP_RECEIVED_HEADERS application
 * callback, to inform it about a delivered HTTP request and allow
 * it to analyze the returned headers.
 **/
typedef struct {
	/*< public >*/
	GnomeVFSURI *uri;
	GList *headers;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackReceivedHeadersIn;

/**
 * GnomeVFSModuleCallbackReceivedHeadersOut:
 * @dummy: unused.
 *
 * A #GnomeVFSModuleCallbackReceivedHeadersOut pointer is passed to a
 * #GNOME_VFS_MODULE_CALLBACK_HTTP_RECEIVED_HEADERS application
 * callback, and is reserved for future use. Applications are not
 * expected to modify this data, because its interpretation might
 * change in the future.
 **/
typedef struct {
	/*< public >*/
	int dummy;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackReceivedHeadersOut;

/**
 * GNOME_VFS_MODULE_CALLBACK_STATUS_MESSAGE:
 *
 * A module callback name used together with gnome_vfs_module_callback_invoke() to
 * inform the user about an ongoing operation.
 *
 * Called when a #GnomeVFSModule has a status message to return to the user.
 *
 * <variablelist role="params">
 * <varlistentry><term><parameter>in arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackStatusMessageIn * containing the message to present to the user.
 * </simpara>
 * </listitem>
 * </varlistentry>
 * <varlistentry><term><parameter>out arg</parameter>&nbsp;:</term>
 * <listitem>
 * <simpara>
 * A #GnomeVFSModuleCallbackStatusMessageOut * (not used).
 * </simpara>
 * </listitem>
 * </varlistentry>
 * </variablelist>
 **/
#define GNOME_VFS_MODULE_CALLBACK_STATUS_MESSAGE "status-message"

/**
 * GnomeVFSModuleCallbackStatusMessageIn:
 * @uri: The textual URI the status message refers to.
 * @message: The message the application should display to the user,
 * indicating the current state, or it is %NULL.
 * @percentage: The percentage indicating the completeness of
 * a currently pending operation on @uri (1-100), or -1 if there
 * is no progress percentage to report.
 *
 * A #GnomeVFSModuleCallbackStatusMessageIn pointer is passed to a
 * #GNOME_VFS_MODULE_CALLBACK_STATUS_MESSAGE application callback,
 * to inform it about the progress and/or status of an ongoing operation.
 **/
typedef struct {
	/*< public >*/
	char *uri;
	char *message;
	int percentage;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackStatusMessageIn;

/**
 * GnomeVFSModuleCallbackStatusMessageOut:
 * @dummy: unused.
 *
 * A #GnomeVFSModuleCallbackStatusMessageOut pointer is passed to a
 * #GNOME_VFS_MODULE_CALLBACK_STATUS_MESSAGE application callback,
 * and is reserved for future use. Applications are not expected
 * to modify this data, because its interpretation might change
 * in the future.
 **/
typedef struct {
	/*< public >*/
	/* empty structs not allowed */
	int dummy;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackStatusMessageOut;

/**
 * GnomeVFSModuleCallbackQuestionIn:
 * @primary_message: A short message explaining the question to the user,
 * or %NULL if there is no message.
 * @secondary_message:  The long version of the message, containing more
 * details, or %NULL if there is no message.
 * @choices: %NULL-terminated string list of choices the user has to choose from.
 * The first item in the list should be affermative, and will be put on the right
 * in a GUI dialog.
 *
 * A #GnomeVFSModuleCallbackQuestionIn pointer is passed to a
 * #GNOME_VFS_MODULE_CALLBACK_QUESTION application callback.
 * Applications are expected to request a decision from the
 * user, and store the answer in a #GnomeVFSModuleCallbackQuestionOut
 * structure.
 **/
typedef struct {
	/*< public >*/
	char *primary_message;
	char *secondary_message;
	char **choices;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackQuestionIn;

/**
 * GnomeVFSModuleCallbackQuestionOut:
 * @answer: The index of the answer the user picked. Matches the base
 * pointer address of the user's choice in #GnomeVFSModuleCallbackQuestionIn,
 * i.e. its index in choices, where the first choice has index %0.
 *
 * A #GnomeVFSModuleCallbackQuestionOut pointer is passed to a
 * #GNOME_VFS_MODULE_CALLBACK_QUESTION application callback, and
 * is used by applications to store the user's decision.
 **/
typedef struct {
	/*< public >*/
	int answer;

	/*< private >*/
	/* Reserved "padding" to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSModuleCallbackQuestionOut;

G_END_DECLS

#endif /* GNOME_VFS_STANDARD_CALLBACKS_H */
