/* Copyright (C) 2000-2004 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation.

   There are special exceptions to the terms and conditions of the GPL as it
   is applied to this software. View the full text of the exception in file
   EXCEPTIONS-CLIENT in the directory of this software distribution.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/* Error messages for MySQL clients */
/* (Error messages for the daemon are in share/language/errmsg.sys) */

#include <my_global.h>
#include <my_sys.h>
#include "errmsg.h"

#ifdef GERMAN
const char *client_errors[]=
{
  "Unbekannter MySQL Fehler",
  "Kann UNIX-Socket nicht anlegen (%d)",
  "Keine Verbindung zu lokalem MySQL Server, socket: '%-.100s' (%d)",
  "Keine Verbindung zu MySQL Server auf %-.100s (%d)",
  "Kann TCP/IP-Socket nicht anlegen (%d)",
  "Unbekannter MySQL Server Host (%-.100s) (%d)",
  "MySQL Server nicht vorhanden",
  "Protokolle ungleich; Server Version = %d, Client Version = %d",
  "MySQL client ran out of memory",
  "Wrong host info",
  "Localhost via UNIX socket",
  "%-.100s via TCP/IP",
  "Error in server handshake",
  "Lost connection to MySQL server during query",
  "Commands out of sync; you can't run this command now",
  "Verbindung ueber Named Pipe: %-.32s",
  "Kann nicht auf Named Pipe warten. Host: %-.64s  pipe: %-.32s (%lu)",
  "Kann Named Pipe nicht oeffnen. Host: %-.64s  pipe: %-.32s (%lu)",
  "Kann den Status der Named Pipe nicht setzen.  Host: %-.64s  pipe: %-.32s (%lu)",
  "Can't initialize character set %-.32s (path: %-.100s)",
  "Got packet bigger than 'max_allowed_packet' bytes",
  "Embedded server",
  "Error on SHOW SLAVE STATUS:",
  "Error on SHOW SLAVE HOSTS:",
  "Error connecting to slave:",
  "Error connecting to master:",
  "SSL connection error",
  "Malformed packet",
  "This client library is licensed only for use with MySQL servers having '%s' license",
  "Invalid use of null pointer",
  "Statement not prepared",
  "No data supplied for parameters in prepared statement",
  "Data truncated",
  "No parameters exist in the statement",
  "Invalid parameter number",
  "Can't send long data for non-string/non-binary data types (parameter: %d)",
  "Using unsupported buffer type: %d  (parameter: %d)",
  "Shared memory: %-.100s",
  "Can't open shared memory; client could not create request event (%lu)",
  "Can't open shared memory; no answer event received from server (%lu)",
  "Can't open shared memory; server could not allocate file mapping (%lu)",
  "Can't open shared memory; server could not get pointer to file mapping (%lu)",
  "Can't open shared memory; client could not allocate file mapping (%lu)",
  "Can't open shared memory; client could not get pointer to file mapping (%lu)",
  "Can't open shared memory; client could not create %s event (%lu)",
  "Can't open shared memory; no answer from server (%lu)",
  "Can't open shared memory; cannot send request event to server (%lu)",
  "Wrong or unknown protocol",
  "Invalid connection handle",
  "Connection using old (pre-4.1.1) authentication protocol refused (client option 'secure_auth' enabled)",
  "Row retrieval was canceled by mysql_stmt_close() call",
  "Attempt to read column without prior row fetch",
  "Prepared statement contains no metadata",
  "Attempt to read a row while there is no result set associated with the statement",
  "This feature is not implemented yet",
  "Lost connection to MySQL server at '%s', system error: %d",
  "Statement closed indirectly because of a preceeding %s() call",
  "The number of columns in the result set differs from the number of bound buffers. You must reset the statement, rebind the result set columns, and execute the statement again",
  "This handle is already connected. Use a separate handle for each connection."
  ""
};

/* Start of code added by Roberto M. Serqueira - martinsc@uol.com.br - 05.24.2001 */

#elif defined PORTUGUESE
const char *client_errors[]=
{
  "Erro desconhecido do MySQL",
  "N�o pode criar 'UNIX socket' (%d)",
  "N�o pode se conectar ao servidor MySQL local atrav�s do 'socket' '%-.100s' (%d)", 
  "N�o pode se conectar ao servidor MySQL em '%-.100s' (%d)",
  "N�o pode criar 'socket TCP/IP' (%d)",
  "'Host' servidor MySQL '%-.100s' (%d) desconhecido",
  "Servidor MySQL desapareceu",
  "Incompatibilidade de protocolos; vers�o do servidor = %d, vers�o do cliente = %d",
  "Cliente do MySQL com falta de mem�ria",
  "Informa��o inv�lida de 'host'",
  "Localhost via 'UNIX socket'",
  "%-.100s via 'TCP/IP'",
  "Erro na negocia��o de acesso ao servidor",
  "Conex�o perdida com servidor MySQL durante 'query'",
  "Comandos fora de sincronismo; voc� n�o pode executar este comando agora",
  "Named pipe: %-.32s",
  "N�o pode esperar pelo 'named pipe' para o 'host' %-.64s - 'pipe' %-.32s (%lu)",
  "N�o pode abrir 'named pipe' para o 'host' %-.64s - 'pipe' %-.32s (%lu)",
  "N�o pode estabelecer o estado do 'named pipe' para o 'host' %-.64s - 'pipe' %-.32s (%lu)",
  "N�o pode inicializar conjunto de caracteres %-.32s (caminho %-.100s)",
  "Obteve pacote maior do que 'max_allowed_packet' bytes",
  "Embedded server"
  "Error on SHOW SLAVE STATUS:",
  "Error on SHOW SLAVE HOSTS:",
  "Error connecting to slave:",
  "Error connecting to master:",
  "SSL connection error",
  "Malformed packet",
  "This client library is licensed only for use with MySQL servers having '%s' license",
  "Invalid use of null pointer",
  "Statement not prepared",
  "No data supplied for parameters in prepared statement",
  "Data truncated",
  "No parameters exist in the statement",
  "Invalid parameter number",
  "Can't send long data for non-string/non-binary data types (parameter: %d)",
  "Using unsupported buffer type: %d  (parameter: %d)",
  "Shared memory: %-.100s",
  "Can't open shared memory; client could not create request event (%lu)",
  "Can't open shared memory; no answer event received from server (%lu)",
  "Can't open shared memory; server could not allocate file mapping (%lu)",
  "Can't open shared memory; server could not get pointer to file mapping (%lu)",
  "Can't open shared memory; client could not allocate file mapping (%lu)",
  "Can't open shared memory; client could not get pointer to file mapping (%lu)",
  "Can't open shared memory; client could not create %s event (%lu)",
  "Can't open shared memory; no answer from server (%lu)",
  "Can't open shared memory; cannot send request event to server (%lu)",
  "Wrong or unknown protocol",
  "Invalid connection handle",
  "Connection using old (pre-4.1.1) authentication protocol refused (client option 'secure_auth' enabled)",
  "Row retrieval was canceled by mysql_stmt_close() call",
  "Attempt to read column without prior row fetch",
  "Prepared statement contains no metadata",
  "Attempt to read a row while there is no result set associated with the statement",
  "This feature is not implemented yet",
  "Lost connection to MySQL server at '%s', system error: %d",
  "Statement closed indirectly because of a preceeding %s() call",
  "The number of columns in the result set differs from the number of bound buffers. You must reset the statement, rebind the result set columns, and execute the statement again",
  "This handle is already connected. Use a separate handle for each connection."
  ""
};

#else /* ENGLISH */
const char *client_errors[]=
{
  "Unknown MySQL error",
  "Can't create UNIX socket (%d)",
  "Can't connect to local MySQL server through socket '%-.100s' (%d)",
  "Can't connect to MySQL server on '%-.100s' (%d)",
  "Can't create TCP/IP socket (%d)",
  "Unknown MySQL server host '%-.100s' (%d)",
  "MySQL server has gone away",
  "Protocol mismatch; server version = %d, client version = %d",
  "MySQL client ran out of memory",
  "Wrong host info",
  "Localhost via UNIX socket",
  "%-.100s via TCP/IP",
  "Error in server handshake",
  "Lost connection to MySQL server during query",
  "Commands out of sync; you can't run this command now",
  "Named pipe: %-.32s",
  "Can't wait for named pipe to host: %-.64s  pipe: %-.32s (%lu)",
  "Can't open named pipe to host: %-.64s  pipe: %-.32s (%lu)",
  "Can't set state of named pipe to host: %-.64s  pipe: %-.32s (%lu)",
  "Can't initialize character set %-.32s (path: %-.100s)",
  "Got packet bigger than 'max_allowed_packet' bytes",
  "Embedded server",
  "Error on SHOW SLAVE STATUS:",
  "Error on SHOW SLAVE HOSTS:",
  "Error connecting to slave:",
  "Error connecting to master:",
  "SSL connection error",
  "Malformed packet",
  "This client library is licensed only for use with MySQL servers having '%s' license",
  "Invalid use of null pointer",
  "Statement not prepared",
  "No data supplied for parameters in prepared statement",
  "Data truncated",
  "No parameters exist in the statement",
  "Invalid parameter number",
  "Can't send long data for non-string/non-binary data types (parameter: %d)",
  "Using unsupported buffer type: %d  (parameter: %d)",
  "Shared memory: %-.100s",
  "Can't open shared memory; client could not create request event (%lu)",
  "Can't open shared memory; no answer event received from server (%lu)",
  "Can't open shared memory; server could not allocate file mapping (%lu)",
  "Can't open shared memory; server could not get pointer to file mapping (%lu)",
  "Can't open shared memory; client could not allocate file mapping (%lu)",
  "Can't open shared memory; client could not get pointer to file mapping (%lu)",
  "Can't open shared memory; client could not create %s event (%lu)",
  "Can't open shared memory; no answer from server (%lu)",
  "Can't open shared memory; cannot send request event to server (%lu)",
  "Wrong or unknown protocol",
  "Invalid connection handle",
  "Connection using old (pre-4.1.1) authentication protocol refused (client option 'secure_auth' enabled)",
  "Row retrieval was canceled by mysql_stmt_close() call",
  "Attempt to read column without prior row fetch",
  "Prepared statement contains no metadata",
  "Attempt to read a row while there is no result set associated with the statement",
  "This feature is not implemented yet",
  "Lost connection to MySQL server at '%s', system error: %d",
  "Statement closed indirectly because of a preceeding %s() call",
  "The number of columns in the result set differs from the number of bound buffers. You must reset the statement, rebind the result set columns, and execute the statement again",
  "This handle is already connected. Use a separate handle for each connection."
  ""
};
#endif


/*
  Register client error messages for use with my_error().

  SYNOPSIS
    init_client_errs()

  RETURN
    void
*/

void init_client_errs(void)
{
  (void) my_error_register(client_errors, CR_ERROR_FIRST, CR_ERROR_LAST);
}


/*
  Unregister client error messages.

  SYNOPSIS
    finish_client_errs()

  RETURN
    void
*/

void finish_client_errs(void)
{
  (void) my_error_unregister(CR_ERROR_FIRST, CR_ERROR_LAST);
}
