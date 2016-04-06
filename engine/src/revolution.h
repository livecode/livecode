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

#ifndef __REVOLUTION__
#define __REVOLUTION__

namespace Revolution
{

class Engine;
class Graphics;
class Control;
class Widget;

struct Rectangle
{
	int32_t x, y;
	int32_t width, height;
};

struct Engine
{
	virtual void *Allocate(uint32_t p_amount) = 0;
	virtual void *Reallocate(void *p_block, uint32_t p_new_size) = 0;
	virtual void Deallocate(void *p_block) = 0;

	virtual uint32_t RegisterWidget(const char *p_id, WidgetConstructorProc p_constructor) = 0;
	virtual void DeregisterWidget(uint32_t p_token) = 0;
};

struct Value
{
	virtual bool GetBoolean(bool& r_value) = 0;
	virtual bool GetReal(double& r_value) = 0;
	virtual bool GetInteger(int32_t& r_value) = 0;
	virtual bool GetCString(const char*& r_value) = 0;
	virtual void GetBuffer(const char*& r_value, int32_t& r_value_length) = 0;
};

struct Container
{
	virtual void SetBoolean(bool p_value) = 0;
	virtual void SetReal(double p_value) = 0;
	virtual void SetInteger(int32_t p_value) = 0;
	virtual void SetCString(const char *p_value) = 0;
	virtual void SetCStringNoCopy(const char *p_value, bool p_free) = 0;
	virtual void SetBuffer(const char *p_value, int32_t p_length) = 0;
	virtual void SetBufferNoCopy(const char *p_value, bool p_free) = 0;
};

struct Control
{
	virtual Rectangle GetBounds(void) = 0;
};

struct Graphics
{
	virtual void Save(void) = 0;
	virtual void Restore(void) = 0;

	virtual void Begin(void) = 0;
	virtual void End(void) = 0;

	virtual void ClipRectangle(const Rectangle& p_rectangle) = 0;
};

struct Widget
{
	virtual void Create(Engine *p_engine, Control *p_control) = 0;
	virtual void Destroy(Engine *p_engine, Control *p_control) = 0;

	virtual void Open(Engine *p_engine, Control *p_control) = 0;
	virtual void Close(Engine *p_engine, Control *p_control) = 0;

	virtual void Resize(Engine *p_engine, Control *p_control) = 0;
	virtual void Render(Engine *p_engine, Control *p_control, Graphics *p_graphics, const Rectangle& p_dirty) = 0;
	virtual bool Touches(Engine *p_engine, Control *p_control, const Rectangle& p_area) = 0;

	virtual bool SetProperty(Engine *p_engine, Control *p_control, const char *p_property, Value *p_key, Value *p_value) = 0;
	virtual bool GetProperty(Engine *p_engine, Control *p_control, const char *p_property, Value *p_key, Container *p_result) = 0;

	virtual bool Pickle(Engine *p_engine, Control *p_control, void*& r_data, int32_t& r_data_length) = 0;
	virtual bool Unpickle(Engine *p_engine, Control *p_control, void *p_data, int32_t p_data_length) = 0;
};

#endif
