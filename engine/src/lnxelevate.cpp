/* Copyright (C) 2003-2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "util.h"
#include "globals.h"

#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <langinfo.h>

////////////////////////////////////////////////////////////////////////////////

static bool make_tmp_fifo_pair(char*& r_name)
{
	bool t_success;
	t_success = true;

	MCAutoPointer<char[]> t_name = strclone("/tmp/revtalk-XXXXXX");

	if (t_success &&
		mkdtemp(*t_name) == nil)
		t_success = false;

	char t_input_fifo_name[64], t_output_fifo_name[64];
	bool t_has_input_fifo, t_has_output_fifo;
	t_has_input_fifo = false;
	t_has_output_fifo = false;
	if (t_success)
	{
		sprintf(t_input_fifo_name, "%s/input", *t_name);
		sprintf(t_output_fifo_name, "%s/output", *t_name);

		t_has_input_fifo = mkfifo(t_input_fifo_name, 0600) != -1;
		t_has_output_fifo = mkfifo(t_output_fifo_name, 0600) != -1;

		t_success = t_has_input_fifo && t_has_output_fifo;
	}

	if (t_success)
		r_name = t_name.Release();
	else
	{
		if (t_has_input_fifo)
			unlink(t_input_fifo_name);
		if (t_has_output_fifo)
			unlink(t_output_fifo_name);
	}

	return t_success;
}

static void unlink_tmp_fifo_pair(const char *p_name)
{
	char t_fifo_name_a[64], t_fifo_name_b[64];
	sprintf(t_fifo_name_a, "%s/input", p_name);
	sprintf(t_fifo_name_b, "%s/output", p_name);
	unlink(t_fifo_name_a);
	unlink(t_fifo_name_b);
	rmdir(p_name);
}

static int32_t open_to_slave(const char *p_name, int p_flags, pid_t p_slave)
{
	for(;;)
	{
		// Try to open the filename, this will be interrupted if a signal occurs
		int t_fd;
		t_fd = open(p_name, p_flags);
		if (t_fd != -1)
			return t_fd;

		// If the open failed and it wasn't interrupted then there was an error
		if (errno != EINTR)
			break;

		// Reset the alarm global.
		MCalarm = False;

		// If the open failed due to a SIGCHLD, and our slave is still there just
		// try again.
		if (waitpid(p_slave, nil, WNOHANG) == 0)
			continue;

		break;
	}

	return -1;
}

static bool open_tmp_fifo_pair(const char *p_name, pid_t p_slave, int32_t& r_input_fd, int32_t& r_output_fd)
{
	char t_input_fifo_name[64], t_output_fifo_name[64];
	sprintf(t_input_fifo_name, "%s/input", p_name);
	sprintf(t_output_fifo_name, "%s/output", p_name);

	int t_input_fd, t_output_fd;
	t_input_fd = -1;
	t_output_fd = -1;

	if (p_slave != -1)
	{
		struct sigaction t_action;
		sigaction(SIGALRM, nil, &t_action);
		t_action . sa_flags &= ~SA_RESTART;
		sigaction(SIGALRM, &t_action, nil);

		t_input_fd = open_to_slave(t_input_fifo_name, O_RDONLY, p_slave);
		if (t_input_fd != -1)
			t_output_fd = open_to_slave(t_output_fifo_name, O_WRONLY, p_slave);

		t_action . sa_flags |= SA_RESTART;
		sigaction(SIGALRM, &t_action, nil);
	}
	else
	{
		t_output_fd = open(t_input_fifo_name, O_WRONLY);
		if (t_output_fd != -1)
			t_input_fd = open(t_output_fifo_name, O_RDONLY);
	}

	if (t_input_fd != -1 && t_output_fd != -1)
	{
		r_input_fd = t_input_fd;
		r_output_fd = t_output_fd;
		return true;
	}

	if (t_input_fd != -1)
		close(t_input_fd);
	if (t_output_fd != -1)
		close(t_output_fd);

	return false;
}

static bool write_uint32_to_fd(int fd, uint32_t value)
{
	value = MCSwapInt32HostToNetwork(value);
	if (write(fd, &value, sizeof(uint32_t)) != sizeof(uint32_t))
		return false;
	return true;
}

static bool write_cstring_to_fd(int fd, char *string)
{
	if (!write_uint32_to_fd(fd, strlen(string) + 1))
		return false;
	if (write(fd, string, strlen(string) + 1) != (signed)(strlen(string) + 1))
		return false;
	return true;
}

static bool read_int32_from_fd(int fd, int32_t& r_value)
{
	uint32_t value;
	if (read(fd, &value, sizeof(int32_t)) != sizeof(int32_t))
		return false;
	r_value = MCSwapInt32NetworkToHost(value);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// This method attempts to launch the given process with the permissions of an
// administrator. At the moment we require libgksu, but should be able to extend
// this to any platform with a suitably configured 'sudo' in the future.
bool MCSystemOpenElevatedProcess(MCStringRef p_command, int32_t& r_pid, int32_t& r_input_fd, int32_t& r_output_fd)
{
	bool t_success;
	t_success = true;

	// Convert the command string into the system encoding so that we can pass
	// Unicode unscathed (hopefully)
	MCAutoPointer<char> t_command;
    size_t t_command_len;
	/* UNCHECKED */ MCStringConvertToSysString(p_command, &t_command, t_command_len);
	
	// First split the command args into the argc/argv array we need.
	char **t_argv;
	uint32_t t_argc;
	t_argv = nil;
	t_argc = 0;
	if (t_success)
		t_success = MCCStringTokenize(*t_command, t_argv, t_argc);

	MCAutoPointer<char[]> t_fifo_name;
	if (t_success)
		t_success = make_tmp_fifo_pair(&t_fifo_name);
		
	// Next we fork so that we can run gksu in a separate process thus allowing
	// us to process things side-by-side.
	pid_t t_pid;
	t_pid = -1;
	if (t_success)
	{
		t_pid = fork();
		if (t_pid == -1)
			t_success = false;
	}

	// If the bootstrap pid is 0 then we are in the child process
	if (t_pid == 0)
	{
		// We must escape MCcmd to make gksu plays nice.
        MCAutoPointer<char> t_unescaped_cmd;
        MCAutoArray<char> t_escaped_cmd;
		size_t t_unescaped_len;
		uindex_t t_escaped_len = 0;
		
		if (t_success)
			t_success = MCStringConvertToSysString(MCcmd, &t_unescaped_cmd, t_unescaped_len);
		
		// The escaping can potentially double the length of the command
		if (t_success)
			t_success = t_escaped_cmd.New(2 * t_unescaped_len + 1);

		if (t_success)
		{
			for (uindex_t i = 0; i < t_unescaped_len; i++)
				if ((*t_unescaped_cmd)[i] == ' ')
				{
					t_escaped_cmd[t_escaped_len++] = '\\';
					t_escaped_cmd[t_escaped_len++] = ' ';
				}
				else
					t_escaped_cmd[t_escaped_len++] = (*t_unescaped_cmd)[i];
			t_escaped_cmd[t_escaped_len] = '\0';
		}

		// Construct the command line for the bootstrap.
		MCAutoPointer<char> t_command_line;
		if (t_success)
			t_success = MCCStringFormat(&t_command_line, "%s -elevated-slave \"%s\"", t_escaped_cmd.Ptr(), *t_fifo_name);

		// We exec to gksu with appropriate parameters.
		// This causes the child to request password from the user and then
		// launch the specified command as admin.
		const char * t_argv[4];
		t_argv[0] = "gksu";
		t_argv[1] = "--preserve-env";
		t_argv[2] = *t_command_line;
		t_argv[3] = nil;

		// Shouldn't return.
		execvp(t_argv[0], (char * const *) t_argv);

		// If we get here an error occurred. We just exit with '-1' since the parent
		// will detect termination of the child.
		_exit(-1);
	}

	// Now that we (hopefully) have a child process, we open the input and then
	// output fifos. Here we pass the pid of the slave so we can check for termination.
	int32_t t_input_fd, t_output_fd;
	t_input_fd = -1;
	t_output_fd = -1;
	if (t_success)
		t_success = open_tmp_fifo_pair(*t_fifo_name, t_pid, t_input_fd, t_output_fd);

	// The child will have hopefully forked and set back its pid. If not, it
	// will have sent back -1.
	int32_t t_admin_pid;
	t_admin_pid = -1;
	if (t_success)
		t_success = read_int32_from_fd(t_input_fd, t_admin_pid);

	// If the pid returned was -1 then something went wrong with the child startup.
	if (t_success && t_admin_pid == -1)
	{
		t_success = false;
	}

	// If we succeeded in launching the child, then it will now be waiting for
	// the actual command line and environment vars.
	if (t_success)
		t_success = write_uint32_to_fd(t_output_fd, t_argc);
	if (t_success)
	{
		for(uint32_t i = 0; i < t_argc && t_success; i++)
		{
			// String tokenization leaves the quotes in - these aren't needed when passed as 
			// arguments so, we adjust.
			char *t_param;
			t_param = t_argv[i];
			if (*t_param == '"')
			{
				t_param[MCCStringLength(t_param) - 1] = '\0';
				t_param += 1;
			}

			t_success = write_cstring_to_fd(t_output_fd, t_param);
		}
	}

	if (t_success)
		t_success = read_int32_from_fd(t_input_fd, t_admin_pid);

	if (t_success)
	{
		r_pid = t_pid;
		r_input_fd = t_input_fd;
		r_output_fd = t_output_fd;
	}
	else
	{
		if (t_input_fd != -1)
			close(t_input_fd);
		if (t_output_fd != -1)
			close(t_output_fd);
	}

	// If tmp fifo pair was created, we can now unlink their files regardless
	if (*t_fifo_name != nil)
		unlink_tmp_fifo_pair(*t_fifo_name);

	MCCStringArrayFree(t_argv, t_argc);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static bool read_uint32_from_fd(int fd, uint32_t& r_value)
{
	uint32_t t_value;
	if (read(fd, &t_value, sizeof(uint32_t)) != sizeof(uint32_t))
		return false;
	r_value = MCSwapInt32NetworkToHost(t_value);
	return true;
}

static bool read_cstring_from_fd(int fd, char*& r_cstring)
{
	uint32_t t_length;
	if (!read_uint32_from_fd(fd, t_length))
		return false;
	
	char *t_string;
	t_string = (char *)malloc(t_length);
	if (read(fd, t_string, t_length) != t_length)
	{
		free(t_string);
		return false;
	}
	
	r_cstring = t_string;
	
	return true;
}

int MCSystemElevatedMain(int argc, char* argv[])
{
	// NOTE: the real arguments are passed through the temporary FIFOs so there
	// is no point asking for the StringRef argv array; it would just have to
	// be converted back to the system encoding...
	
	char *t_fifo_name;
	t_fifo_name = argv[2];

	// Make sure that argv[2] is unquoted. It seems that some version of gksu
	// will unquote, others won't.
	if (t_fifo_name[0] == '"')
	{
		t_fifo_name += 1;
		t_fifo_name[strlen(t_fifo_name) - 1] = '\0';
	}

	// Open the fifos (in slave mode - 2nd parameter == -1)
	int t_output_fifo, t_input_fifo;
	if (!open_tmp_fifo_pair(t_fifo_name, -1, t_input_fifo, t_output_fifo))
		return -1;

	// Swap the input fifo into stdin
	close(0);
	dup(t_input_fifo);
	close(t_input_fifo);

	// Swap the output fifo into stdout
	close(1);
	dup(t_output_fifo);
	close(t_output_fifo);

	// Here we are in the child, so we can send our pid along
	write_uint32_to_fd(fileno(stdout), getpid());

	// Now we can read in the arguments
	uint32_t t_arg_count;
	if (!read_uint32_from_fd(fileno(stdin), t_arg_count))
		return -1;

	// The arguments read from the FIFO are encoded in the system encoding
	char **t_args;
	t_args = (char **)malloc(sizeof(char *) * (t_arg_count + 1));
	memset(t_args, 0, (t_arg_count + 1) * sizeof(char *));
	for(uint32_t i = 0; i < t_arg_count; i++)
	{
		if (!read_cstring_from_fd(fileno(stdin), t_args[i]))
			break;
	}
	t_args[t_arg_count] = nil;

	// Here we are in the child, so we can send our pid along
	write_uint32_to_fd(fileno(stdout), getpid());

	// And exec to the new process image
	execvp(t_args[0], t_args);

	// we will only reach here if the execvp call fails
	free(t_args);
	return -1;
}
