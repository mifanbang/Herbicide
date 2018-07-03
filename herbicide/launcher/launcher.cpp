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

#include <algorithm>
#include <string>

#include <windows.h>
#include <shlwapi.h>

#include <gandr/Buffer.h>
#include <gandr/Handle.h>

#include "shared/herbicide.h"
#include "shared/util.h"
#include "payload.h"



namespace {



// output localized error message if $dwErrCode is non-zero
void ShowErrorMessageBox(LPCWSTR lpszMsg, DWORD dwErrCode)
{
	std::wstring message = L"An error occurred during launching.\n";

	if (dwErrCode != NO_ERROR) {
		gan::AutoHandle systemMsg(static_cast<LPWSTR>(nullptr), ::LocalFree);
		::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			nullptr,
			dwErrCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPWSTR>(&systemMsg.GetRef()),
			0,
			nullptr
		);

		message.reserve(wcslen(systemMsg) + wcslen(lpszMsg) + 128);
		message += L"\nFunction: ";
		message += lpszMsg;
		message += L"\nCode: ";
		message += std::to_wstring(dwErrCode);
		message += L"\nDetail: ";
		message += systemMsg;
	}
	else {
		message.reserve(wcslen(lpszMsg) + 128);
		message += L"\nDetail: ";
		message += lpszMsg;
	}

	::MessageBox(nullptr, message.c_str(), c_appName, MB_OK | MB_ICONERROR);
}


// return true on success; return false otherwise
bool UnpackPayloadTo(const std::wstring& path)
{
	auto lpszPath = path.c_str();
	bool bShouldUnpack = true;
	bool bSucceeded = false;

	// check for path
	bShouldUnpack = bShouldUnpack && !PathFileExists(lpszPath);

	// match the hash of payload with that of an pre-existing file
	bShouldUnpack = bShouldUnpack || !CheckFileHash(lpszPath, s_payloadHash);

	if (bShouldUnpack) {
		DWORD dwPayloadSize = sizeof(s_payloadData);
		auto payloadData = gan::Buffer::Allocate(dwPayloadSize);
		if (payloadData == nullptr)
			return false;
		CopyMemory(*payloadData, s_payloadData, dwPayloadSize);

		// de-obfuscate our code
		for (DWORD i = 0; i < dwPayloadSize; i++)
			(*payloadData)[i] ^= c_byteObfuscator;

		// write to a temp path
		gan::AutoWinHandle hFile = ::CreateFile(lpszPath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		DWORD dwWritten;
		if (hFile != INVALID_HANDLE_VALUE)
		{
			::WriteFile(hFile, *payloadData, dwPayloadSize, &dwWritten, nullptr);
			bSucceeded = true;
		}
	}
	else
		bSucceeded = true;  // file already exists

	return bSucceeded;
}



}  // unnames namespace



int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
#ifdef _DEBUG
	DebugConsole dbgConsole;
#endif  // _DEBUG

	// generate DLL path in user's Temp directory
	auto pathPayload = GetPayloadPath();
	DEBUG_MSG(L"Payload path: %s\n", pathPayload.c_str());
	if (!UnpackPayloadTo(pathPayload)) {
		ShowErrorMessageBox(L"UnpackPayloadTo()", GetLastError());
		return 0;
	}

	// get executable paths
	auto pathDir = GetMirrorDir();
	auto pathExe = pathDir + L"/game.exe";
	if (pathExe.empty()) {
		ShowErrorMessageBox(L"Failed to locate install directory from registry", NO_ERROR);
		return 0;  // according to MSDN, we should return zero before entering the message loop
	}
	DEBUG_MSG(L"EXE path: %s\n", pathExe.c_str());

	// create and purify
	auto createdPid = CreatePurifiedProcess(pathExe.c_str(), pathDir.c_str(), pathPayload.c_str());
	if (createdPid == 0) {
		auto errCode = GetLastError();
		ShowErrorMessageBox(L"CreatePurifiedProcess()", errCode);
		return errCode;
	}

	return NO_ERROR;
}

