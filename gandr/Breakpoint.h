/*
 *  herbicide - removing flowers and rabbits in the game Mirror
 *  Copyright (C) 2018 Mifan Bang <https://debug.tw>.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <windows.h>



namespace gan {



// ---------------------------------------------------------------------------
// class HWBreakpoint32 - hardware breakpoint on execution
// ---------------------------------------------------------------------------

class HWBreakpoint32
{
public:
	static bool Enable(HANDLE hThread, LPVOID pAddress, unsigned int nSlot) noexcept;
	static bool Disable(HANDLE hThread, unsigned int nSlot) noexcept;
};



}  // namespace gan
