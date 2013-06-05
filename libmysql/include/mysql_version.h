/* Copyright Abandoned 1996, 1999, 2001 MySQL AB
   This file is public domain and comes with NO WARRANTY of any kind */

/* Version numbers for protocol & mysqld */

#ifndef _mysql_version_h
#define _mysql_version_h

#ifdef _CUSTOMCONFIG_
# include <custom_conf.h>
#else
# define PROTOCOL_VERSION		10
# define MYSQL_SERVER_VERSION		"6.0.0"
# define MYSQL_VERSION_ID		60000
# define MYSQL_PORT			3306
# define MYSQL_PORT_DEFAULT		0
# define MYSQL_UNIX_ADDR		"/tmp/mysql.sock"
# define MYSQL_CONFIG_NAME		"my"
# define MYSQL_COMPILATION_COMMENT	"Source distribution"
#endif /* _CUSTOMCONFIG_ */

#ifndef LICENSE
# define LICENSE			GPL
#endif /* LICENSE */

#endif /* _mysql_version_h */
