/* Copyright (C) 2017 LiveCode Ltd.

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

#ifndef MC_TRISTATE_H
#define MC_TRISTATE_H

/* In a few places in the engine, a type is needed that can be true,
 * false, or somewhere between the two.  This is provided by the
 * MCTristate type.  The values in the MCTristateValue enumeration
 * shouldn't be used except to implement a switch over an MCTristate.
 *
 * Always use an MCTristate as a value (just like bool or int).
 */

enum MCTristateValue {
    kMCTristateFalse = 0,
    kMCTristateTrue = 1,
    kMCTristateMixed = 2,
};

class MCTristate {
 public:
    MCTristate() : value(kMCTristateFalse) {}

    MCTristate(const MCTristate& t) : value (t.value) {}

    MCTristate(MCTristateValue v) : value(v) {}

    MCTristate(bool b) : value(b ? kMCTristateTrue : kMCTristateFalse) {}

    bool isFalse() const { return value == kMCTristateFalse; }

    bool isMixed() const { return value == kMCTristateMixed; }

    bool isTrue() const { return value == kMCTristateTrue; }

    bool operator==(const MCTristate& t) const
    {
        return value == t.value;
    }

    bool operator!=(const MCTristate& t) const
    {
        return !operator==(t);
    }

    MCTristate& operator=(const MCTristate& t)
    {
        value = t.value;
        return *this;
    }

    MCTristateValue value;
};

#endif /* !MC_TRISTATE_H */
