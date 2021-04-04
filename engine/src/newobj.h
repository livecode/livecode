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

//
// create new script objects
//
#ifndef	NEWOBJ_H
#define	NEWOBJ_H

class MCStatement;
class MCExpression;

/* The MCNewObjFactory class is designed to be derived from to form the three
 * lists of newobj functions for statements, functions and operators.
 *
 * Derivations of the class are declared as global instances which automatically
 * hook themselves into a globally-linked list via global constructors.
 *
 * As there is no guaranteed order to global constructors, care
 * must be taken to ensure the syntax ids handled by each registerd function are
 * distinct.
 *
 * The template base class exploits the fact that template arguments can be
 * a derived class. This allows all the code to be in the base-class, with
 * derived classes only needing to add a class variable `s_functions` which
 * holds the root of the list and a constructor. */
template<typename Derived, typename Node>
class MCNewObjFactory
{
public:
	typedef Derived Self;
	typedef MCNewObjFactory<Derived, Node> Base;
	typedef Node *(*Function)(int2 p_which);

	MCNewObjFactory(
			Function p_function)
		: m_function(p_function)
	{
		m_next = Derived::s_functions;
		Derived::s_functions = this;
	}

	static Node *
	New(int2 p_which)
	{
		if (Derived::s_functions == nullptr)
		{
			return nullptr;
		}
		return Derived::s_functions->_New(p_which);
	}

private:
	Node *_New(
			int2 p_which)
	{
		Node *t_instance
			= m_function(p_which);
		if (t_instance != nullptr)
		{
			return t_instance;
		}

		if (m_next == nullptr)
		{
			return nullptr;
		}

		return m_next->New(p_which);
	}

	Base *m_next;
	Function m_function;
};

/* The MCNewStatementFactory class should be globally instantiated with
 * a factory function for statement ids. */
class MCNewStatementFactory
	: public MCNewObjFactory<MCNewStatementFactory, MCStatement>
{
public:
	MCNewStatementFactory(
			Function p_function)
		: Base(p_function)
	{
	}

private:
	static Base *s_functions;
	friend Base;
};

/* The MCNewFunctionFactory class should be globally instantiated with a factory
 * function for function ids. */
class MCNewFunctionFactory
	: public MCNewObjFactory<MCNewFunctionFactory, MCExpression>
{
public:
	MCNewFunctionFactory(
			Function p_function)
		: Base(p_function)
	{
	}

private:
	static Base *s_functions;
	friend Base;
};

/* The MCNewOperatorFactory class should be globally instantiated with a factory
 * function for operator ids. */
class MCNewOperatorFactory
	: public MCNewObjFactory<MCNewOperatorFactory, MCExpression>
{
public:
	MCNewOperatorFactory(
			Function p_function)
		: Base(p_function)
	{
	}

private:
	static Base *s_functions;
	friend Base;
};

/* The new methods for each type of syntax id which can be instantiated. These
 * functions handle the main syntax ids first, and then pass on responsibility
 * to the list of factories for the syntax type. */
extern MCStatement *MCN_new_statement(int2 which);
extern MCExpression *MCN_new_function(int2 which);
extern MCExpression *MCN_new_operator(int2 which);

#endif
