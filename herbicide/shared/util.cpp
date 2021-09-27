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

#include <functional>
#include <memory>

#include <windows.h>

#include <Debugger.h>
#include <DllPreloadDebugSession.h>
#include <Handle.h>

#include "herbicide.h"

#include "util.h"


namespace {



const std::wstring& GetPayloadFileName()
{
	static std::wstring fname = std::wstring(c_appName) + L"_" + c_appVersion + L".dll";
	return fname;
}


}  // unnamed namespace



// ---------------------------------------------------------------------------
// debug utilities
// ---------------------------------------------------------------------------

DebugConsole::DebugConsole()
{
	FILE* fp;
	::AllocConsole();
	freopen_s(&fp, "CONIN$", "r+t", stdin);
	freopen_s(&fp, "CONOUT$", "w+t", stdout);
	freopen_s(&fp, "CONOUT$", "w+t", stderr);
}


DebugConsole::~DebugConsole()
{
	puts("I'm done");
	system("pause");

	::FreeConsole();
}


// ---------------------------------------------------------------------------
// hash functions
// ---------------------------------------------------------------------------

std::unique_ptr<gan::Buffer> ReadFileToBuffer(const wchar_t* lpPath, WinErrorCode& errCode)
{
	gan::AutoWinHandle hFile = ::CreateFile(lpPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, nullptr);
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD dwSizePayload = ::GetFileSize(hFile, nullptr);

		auto fileContent = gan::Buffer::Allocate(dwSizePayload);
		DWORD dwSizeRead;
		if (fileContent != nullptr && ::ReadFile(hFile, *fileContent, dwSizePayload, &dwSizeRead, nullptr) == TRUE) {
			errCode = NO_ERROR;
			return std::move(fileContent);
		}
	}

	errCode = ::GetLastError();
	return std::unique_ptr<gan::Buffer>();
}


bool CheckFileHash(const wchar_t* lpszPath, const gan::Hash<256>& hash)
{
	std::unique_ptr<gan::Buffer> fileContent;
	WinErrorCode errCode;
	gan::Hash<256> hashFileOnDisk;

	bool bDoHashesMatch = true;
	bDoHashesMatch = bDoHashesMatch && (fileContent = ReadFileToBuffer(lpszPath, errCode)).get() != nullptr;
	bDoHashesMatch = bDoHashesMatch && gan::Hasher::GetSHA(*fileContent, fileContent->GetSize(), hashFileOnDisk) == NO_ERROR;
	bDoHashesMatch = bDoHashesMatch && hashFileOnDisk == hash;

	return bDoHashesMatch;
}


// ---------------------------------------------------------------------------
// process creation function
// ---------------------------------------------------------------------------

namespace {

#ifdef _DEBUG
	// just for the purpose to override OnPreEvent()
	class PurifierDLLPreloadDebugSession : public gan::DllPreloadDebugSession
	{
	public:
		PurifierDLLPreloadDebugSession(const CreateProcessParam& newProcParam, const wchar_t* pPayloadPath, Option option)
			: gan::DllPreloadDebugSession(newProcParam, pPayloadPath, option)
		{ }

	private:
		virtual void OnPreEvent(const PreEvent& event) override
		{
			DEBUG_MSG(L"Event: 0x%x\n", event.eventCode);
		}
	};

#else
	using PurifierDLLPreloadDebugSession = gan::DllPreloadDebugSession;

#endif  // _DEBUG
}  // unnamed namespace


uint32_t CreatePurifiedProcess(const wchar_t* szExePath, const wchar_t* szCurrDir, const wchar_t* szPayloadPath)
{
	gan::Debugger debugger;

	gan::DebugSession::CreateProcessParam createParam;
	createParam.imagePath = szExePath;
	createParam.currentDir = szCurrDir;
	if (!debugger.AddSession<PurifierDLLPreloadDebugSession>(createParam, szPayloadPath, gan::DllPreloadDebugSession::Option::EndSessionSync).lock())
		return 0;

	// cache pid
	gan::Debugger::IdList pidList;
	debugger.GetSessionList(pidList);

	if (debugger.EnterEventLoop() == gan::Debugger::EventLoopResult::ErrorOccurred)
		return 0;

	return pidList[0];
}


// ---------------------------------------------------------------------------
// misc functions
// ---------------------------------------------------------------------------

std::wstring GetPayloadPath()
{
	WCHAR buffer[MAX_PATH];
	::GetTempPathW(sizeof(buffer) / sizeof(buffer[0]), buffer);
	return std::wstring(buffer) + GetPayloadFileName();
}


std::wstring GetMirrorDir()
{
	std::wstring output;

	HKEY hRegKey;
	DWORD dwSize = MAX_PATH;
	wchar_t szPath[MAX_PATH];

	const auto regOpenResult = ::RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 644560",
		REG_OPTION_OPEN_LINK,
		KEY_READ | KEY_WOW64_64KEY,
		&hRegKey
	);
	if (regOpenResult == NO_ERROR) {
		if (::RegQueryValueExW(hRegKey, L"InstallLocation", nullptr, nullptr, reinterpret_cast<PBYTE>(szPath), &dwSize) == NO_ERROR)
			output = szPath;
		::RegCloseKey(hRegKey);
	}

	return output;
}
