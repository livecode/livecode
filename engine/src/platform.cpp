#include "platform.h"
#include "platform-internal.h"

////////////////////////////////////////////////////////////////////////////////

void MCPlatformHandleApplicationStartup(int p_argc, char **p_argv, char **p_envp, int& r_error_code, char*& r_error_message);
void MCPlatformHandleApplicationShutdown(int& r_exit_code);
void MCPlatformHandleApplicationShutdownRequest(bool& r_terminate);
void MCPlatformHandleApplicationRun(void);

void MCPlatformHandleWindowCloseRequest(MCPlatformWindowRef window);
void MCPlatformHandleWindowIconify(MCPlatformWindowRef window);
void MCPlatformHandleWindowUniconify(MCPlatformWindowRef window);
void MCPlatformHandleWindowReshape(MCPlatformWindowRef window);
void MCPlatformHandleWindowFocus(MCPlatformWindowRef window);
void MCPlatformHandleWindowUnfocus(MCPlatformWindowRef window);
void MCPlatformHandleWindowRedraw(MCPlatformWindowRef window, MCPlatformSurfaceRef surface, MCRegionRef dirty_rgn);

void MCPlatformHandleMouseEnter(MCPlatformWindowRef window);
void MCPlatformHandleMouseLeave(MCPlatformWindowRef window);
void MCPlatformHandleMouseDown(MCPlatformWindowRef window, uint32_t button, uint32_t count);
void MCPlatformHandleMouseUp(MCPlatformWindowRef window, uint32_t button, uint32_t count);
void MCPlatformHandleMouseRelease(MCPlatformWindowRef window, uint32_t button);
void MCPlatformHandleMouseMove(MCPlatformWindowRef window, MCPoint location);

void MCPlatformHandleDragStart(MCPlatformWindowRef window);
void MCPlatformHandleDragEnter(MCPlatformWindowRef window);
void MCPlatformHandleDragMove(MCPlatformWindowRef window, MCPoint location);
void MCPlatformHandleDragLeave(MCPlatformWindowRef window);
void MCPlatformHandleDragEnd(MCPlatformWindowRef window);

void MCPlatformHandleMenuUpdate(MCPlatformMenuRef menu);
void MCPlatformHandleMenuSelect(MCPlatformMenuRef menu, uindex_t index);

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCallbackSendApplicationStartup(int p_argc, char **p_argv, char **p_envp, int& r_error_code, char*& r_error_message)
{
	MCPlatformHandleApplicationStartup(p_argc, p_argv, p_envp, r_error_code, r_error_message);
}

void MCPlatformCallbackSendApplicationShutdown(int& r_exit_code)
{
	MCPlatformHandleApplicationShutdown(r_exit_code);
}

void MCPlatformCallbackSendApplicationShutdownRequest(bool& r_terminate)
{
	MCPlatformHandleApplicationShutdownRequest(r_terminate);
}

void MCPlatformCallbackSendApplicationRun(void)
{
	MCPlatformHandleApplicationRun();
}

//////////

void MCPlatformCallbackSendWindowCloseRequest(MCPlatformWindowRef p_window)
{
	MCLog("Window(%p) -> CloseRequest()", p_window);
	MCPlatformHandleWindowCloseRequest(p_window);
}

void MCPlatformCallbackSendWindowReshape(MCPlatformWindowRef p_window, MCRectangle p_new_content)
{
	MCLog("Window(%p) -> WindowReshape([%d, %d, %d, %d])", p_window, p_new_content . x, p_new_content . y, p_new_content . width, p_new_content . height);
	MCPlatformHandleWindowReshape(p_window);
}

void MCPlatformCallbackSendWindowRedraw(MCPlatformWindowRef p_window, MCPlatformSurfaceRef p_surface, MCRegionRef p_dirty_rgn)
{
	MCLog("Window(%p) -> WindowRedraw(%p, %p)", p_window, p_surface);
	MCPlatformHandleWindowRedraw(p_window, p_surface, p_dirty_rgn);
}

void MCPlatformCallbackSendWindowIconify(MCPlatformWindowRef p_window)
{
	MCLog("Window(%p) -> WindowIconify()", p_window);
	MCPlatformHandleWindowIconify(p_window);
}

void MCPlatformCallbackSendWindowUniconify(MCPlatformWindowRef p_window)
{
	MCLog("Window(%p) -> WindowUniconify()", p_window);
	MCPlatformHandleWindowUniconify(p_window);
}

void MCPlatformCallbackSendWindowFocus(MCPlatformWindowRef p_window)
{
	MCLog("Window(%p) -> WindowFocus()", p_window);
	MCPlatformHandleWindowFocus(p_window);
}

void MCPlatformCallbackSendWindowUnfocus(MCPlatformWindowRef p_window)
{
	MCLog("Window(%p) -> WindowUnfocus()", p_window);
	MCPlatformHandleWindowUnfocus(p_window);
}

void MCPlatformCallbackSendMouseEnter(MCPlatformWindowRef p_window)
{
	MCLog("Window(%p) -> MouseEnter()", p_window);
	MCPlatformHandleMouseEnter(p_window);
}

void MCPlatformCallbackSendMouseLeave(MCPlatformWindowRef p_window)
{
	MCLog("Window(%p) -> MouseLeave()", p_window);
	MCPlatformHandleMouseLeave(p_window);
}

void MCPlatformCallbackSendMouseDown(MCPlatformWindowRef p_window, uint32_t p_button, uint32_t p_count)
{
	MCLog("Window(%p) -> MouseDown(%d, %d)", p_window, p_button, p_count);
	MCPlatformHandleMouseDown(p_window, p_button, p_count);
}

void MCPlatformCallbackSendMouseUp(MCPlatformWindowRef p_window, uint32_t p_button, uint32_t p_count)
{
	MCLog("Window(%p) -> MouseUp(%d, %d)", p_window, p_button, p_count);
	MCPlatformHandleMouseUp(p_window, p_button, p_count);
}

void MCPlatformCallbackSendMouseRelease(MCPlatformWindowRef p_window, uint32_t p_button)
{
	MCLog("Window(%p) -> MouseRelease(%d)", p_window, p_button);
	MCPlatformHandleMouseRelease(p_window, p_button);
}

void MCPlatformCallbackSendMouseMove(MCPlatformWindowRef p_window, MCPoint p_location)
{
	MCLog("Window(%p) -> MouseMove([%d, %d])", p_window, p_location . x, p_location . y);
	MCPlatformHandleMouseMove(p_window, p_location);
}

//////////

void MCPlatformCallbackSendMenuUpdate(MCPlatformMenuRef p_menu)
{
	MCLog("Menu(%p) -> Update()", p_menu);
	MCPlatformHandleMenuUpdate(p_menu);
}

void MCPlatformCallbackSendMenuSelect(MCPlatformMenuRef p_menu, uindex_t p_index)
{
	MCLog("Menu(%p) -> Select(%d)", p_menu, p_index);
	MCPlatformHandleMenuSelect(p_menu, p_index);
}

////////////////////////////////////////////////////////////////////////////////

#if 0
void MCPlatformCallbackSendApplicationStartup(int p_argc, char **p_argv, char **p_envp, int& r_error_code, char*& r_error_message)
{
	MCPlatformCallback t_callback;
	t_callback . type = kMCPlatformCallbackApplicationStartup;
	t_callback . application . startup . argc = p_argc;
	t_callback . application . startup . argv = p_argv;
	t_callback . application . startup . envp = p_envp;
	t_callback . application . startup . error_code = 0;
	t_callback . application . startup . error_message = nil;
	MCPlatformProcess(t_callback);
	r_error_code = t_callback . application . startup . error_code;
	r_error_message = t_callback . application . startup . error_message;
}

void MCPlatformCallbackSendApplicationShutdown(int& r_exit_code)
{
	MCPlatformCallback t_callback;
	t_callback . type = kMCPlatformCallbackApplicationShutdown;
	t_callback . application . shutdown . exit_code = 0;
	MCPlatformProcess(t_callback);
	r_exit_code = t_callback . application . shutdown . exit_code;
}

void MCPlatformCallbackSendApplicationShutdownRequest(bool& r_terminate)
{
	MCPlatformCallback t_callback;
	t_callback . type = kMCPlatformCallbackApplicationShutdownRequest;
	t_callback . application . shutdown_request . terminate = false;
	MCPlatformProcess(t_callback);
	r_terminate = t_callback . application . shutdown_request . terminate;
}

void MCPlatformCallbackSendApplicationRun(void)
{
	MCPlatformCallback t_callback;
	t_callback . type = kMCPlatformCallbackApplicationRun;
	MCPlatformProcess(t_callback);
}
#endif

////////////////////////////////////////////////////////////////////////////////
