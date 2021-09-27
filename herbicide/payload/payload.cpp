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

#include <vector>

#include <Hook.h>

#include "shared/herbicide.h"
#include "shared/util.h"
#include "detours/d3d11.h"



class Scenario
{
public:
	Scenario()
		: m_hookList()
	{ }

	void Start()
	{
		for (auto& hook : m_hookList)
			hook.Install();
	}

	void Stop()
	{
		for (auto& hook : m_hookList)
			hook.Uninstall();
	}

protected:
	std::vector<gan::Hook> m_hookList;
};


class ScenarioMirror : public Scenario
{
public:
	ScenarioMirror()
		: Scenario()
	{
		m_hookList.emplace_back(::D3D11CreateDevice, detour::D3D11CreateDevice);
	}
};



BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
	static DebugConsole* pDbgConsole = nullptr;
	static Scenario* s_scenaro = nullptr;

	if (fdwReason == DLL_PROCESS_ATTACH) {
#ifdef _DEBUG
		pDbgConsole = new DebugConsole;
#endif  // _DEBUG

		if (s_scenaro == nullptr) {
			s_scenaro = new ScenarioMirror;
			s_scenaro->Start();
		}
	}
	else if (fdwReason == DLL_PROCESS_DETACH) {
		if (s_scenaro != nullptr) {
			s_scenaro->Stop();
			delete s_scenaro;
			s_scenaro = nullptr;
		}

		if (pDbgConsole != nullptr) {
			delete pDbgConsole;
			pDbgConsole = nullptr;
		}
	}

	return TRUE;
}
