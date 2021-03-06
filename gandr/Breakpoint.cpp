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

#include "Breakpoint.h"



namespace {


DWORD* GetRegisterFromSlot(CONTEXT& ctx, unsigned int nSlot) noexcept
{
	return nSlot < 4 ? &ctx.Dr0 + nSlot : nullptr;
}


DWORD GetMaskFromSlot(unsigned int nSlot) noexcept
{
	if (nSlot < 4)
		return 1 << (nSlot << 1);
	return 0;
}


enum class Dr7UpdateOperation
{
	Enable,
	Disable
};


bool UpdateDebugRegisters(HANDLE hThread, LPVOID pAddress, unsigned int nSlot, Dr7UpdateOperation opDr7) noexcept
{
	if (nSlot >= 4)
		return false;

	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	if (::GetThreadContext(hThread, &ctx) == 0)
		return false;

	DWORD* pReg = GetRegisterFromSlot(ctx, nSlot);
	*pReg = reinterpret_cast<DWORD>(pAddress);
	if (opDr7 == Dr7UpdateOperation::Enable)
		ctx.Dr7 |= GetMaskFromSlot(nSlot);
	else
		ctx.Dr7 &= ~GetMaskFromSlot(nSlot);
	ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	if (::SetThreadContext(hThread, &ctx) == 0)
		return false;

	return true;
}



}  // unnamed namespace



namespace gan {



bool HWBreakpoint32::Enable(HANDLE hThread, LPVOID pAddress, unsigned int nSlot) noexcept
{
	return UpdateDebugRegisters(hThread, pAddress, nSlot, Dr7UpdateOperation::Enable);
}


bool HWBreakpoint32::Disable(HANDLE hThread, unsigned int nSlot) noexcept
{
	return UpdateDebugRegisters(hThread, nullptr, nSlot, Dr7UpdateOperation::Disable);
}



}  // namespace gan
