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

#pragma warning(disable: 4731)  // frame pointer register 'ebp' modified by inline assembly code
#pragma warning(disable: 4740)  // flow in or out of inline asm code suppresses global optimization

#define TEXTURE_DUMPING_MODE	0
#define TEXTURE_DUMPING_LIB		1  // 0 for using legacy D3D11 SDK; 1 for using DirectXTK.

#include "d3d11.h"

#if TEXTURE_DUMPING_MODE
	#if TEXTURE_DUMPING_LIB == 0
		#include <D3DX11tex.h>
		#pragma comment(lib, "d3dx11.lib")
	#elif TEXTURE_DUMPING_LIB == 1
		#include <DXGItype.h>
		#include <wincodec.h>
		#include <../../DirectXTK/Inc/ScreenGrab.h>
		#pragma comment(lib, "../../DirectXTK/Bin/Desktop_2019_Win10/Win32/Release/DirectXTK.lib")
	#else
		#error "Invalid TEXTURE_DUMPING_LIB value"
	#endif  // TEXTURE_DUMPING_LIB
#endif  // TEXTURE_DUMPING_MODE

#include <Hook.h>

#include "shared/util.h"
#include "../TextureFilter.h"


namespace {


ID3D11DeviceContext* s_deviceContext = nullptr;
void* s_addrCreateTexture2D = nullptr;
void* s_addrUnmap = nullptr;
void* s_addrMap = nullptr;

ResourceSuspectList s_suspectList;

#if TEXTURE_DUMPING_MODE
std::unordered_map<ID3D11Resource*, D3D11_MAPPED_SUBRESOURCE> s_mappedRes;

bool DumpTexture(ID3D11DeviceContext* pContext, ID3D11Resource* pResource, bool checkMappedResource)
{
	static bool callFlag = false;  // prevent texture saving function from recursively calling itself
	static uint32_t uniqueId = 0;

	D3D11_RESOURCE_DIMENSION type;
	pResource->GetType(&type);
	if (!callFlag && type == ::D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
		D3D11_TEXTURE2D_DESC desc;
		reinterpret_cast<::ID3D11Texture2D*>(pResource)->GetDesc(&desc);

		gan::Hash<256> hash { };
		if (checkMappedResource) {
			auto itr = s_mappedRes.find(pResource);
			if (itr == s_mappedRes.cend())
				return false;
			auto mapped = itr->second;
			s_mappedRes.erase(pResource);

			if (desc.Height >= 32)
				gan::Hasher::GetSHA(mapped.pData, mapped.RowPitch << 5, hash);
			else
				return false;
		}

		std::wstring path;
		{
			path.reserve(MAX_PATH);
			path.append(L"tx\\tx_");
			for (int i = 0; i < 32; ++i) {
				wchar_t buf[32];
				wsprintf(buf, L"%02X", hash.data[i]);
				path.append(buf);
			}
			path.push_back('_');
			path.append(std::to_wstring(desc.Format));
			path.push_back('_');
			path.append(std::to_wstring(uniqueId++));
			path.append(L".png");
		}

		callFlag = true;
		LRESULT result = S_OK;
#if TEXTURE_DUMPING_LIB == 0
		result = ::D3DX11SaveTextureToFileW(pContext, pResource, ::D3DX11_IFF_PNG, path.c_str());

#elif TEXTURE_DUMPING_LIB == 1
		result = DirectX::SaveWICTextureToFile(pContext, pResource, GUID_ContainerFormatPng, path.c_str(), nullptr, nullptr, true);
		if (result != S_OK)
			result = DirectX::SaveDDSTextureToFile(pContext, pResource, path.c_str());
#endif  // TEXTURE_DUMPING_LIB
		callFlag = false;

		return result == S_OK;
	}

	return false;
}
#endif  // TEXTURE_DUMPING_MODE


HRESULT WINAPI CreateTexture2D(
	[[maybe_unused]]ID3D11Device* pDevice,
	[[maybe_unused]]const D3D11_TEXTURE2D_DESC* pDesc,
	[[maybe_unused]]const D3D11_SUBRESOURCE_DATA* pInitialData,
	[[maybe_unused]]ID3D11Texture2D** ppTexture2D
)
{
	HRESULT result;
	__asm {
		lea eax, pDevice
		push dword ptr[eax + 0Ch]
		push dword ptr[eax + 8h]
		push dword ptr[eax + 4h]
		push dword ptr[eax]
		call Trampoline
		mov result, eax
		jmp ResultAvailable

Trampoline:
		push ebp
		mov ebp, esp
		mov eax, s_addrCreateTexture2D
		add eax, 5
		jmp eax

ResultAvailable:
	}
	if (result != S_OK)
		return result;

	DEBUG_MSG(
		L"CreateTexture2D w=%d h=%d fmt=%d usg=%d dat=%p tex=%p\n",
		pDesc->Width,
		pDesc->Height,
		pDesc->Format,
		pDesc->Usage,
		pInitialData,
		ppTexture2D ? *ppTexture2D : nullptr);

	if (pInitialData != nullptr && s_deviceContext != nullptr)
	{
		DEBUG_MSG(L"  pInitialData: pSysMem=%p SysMemPitch=%d SysMemSlicePitch=%d\n", pInitialData->pSysMem, pInitialData->SysMemPitch, pInitialData->SysMemSlicePitch);
#if TEXTURE_DUMPING_MODE
		DumpTexture(s_deviceContext, *ppTexture2D, false);
#endif
	}

	DataFilterList dataFilters;
	if (GetDataFilterFactory().Match(*pDesc, dataFilters)) {
		ResourceSuspect suspect;
		suspect.filterList = std::move(dataFilters);
		s_suspectList.Add(*ppTexture2D, std::move(suspect));
	}
	return S_OK;
}


HRESULT WINAPI Map(
	[[maybe_unused]] ID3D11DeviceContext* pContext,
	[[maybe_unused]] ID3D11Resource* pResource,
	[[maybe_unused]] UINT Subresource,
	[[maybe_unused]] D3D11_MAP MapType,
	[[maybe_unused]] UINT MapFlags,
	[[maybe_unused]] D3D11_MAPPED_SUBRESOURCE* pMappedResource
)
{
	if (s_deviceContext == nullptr)
		s_deviceContext = pContext;

	HRESULT result;
	__asm {
		lea eax, pContext
		push dword ptr[eax + 14h]
		push dword ptr[eax + 10h]
		push dword ptr[eax + 0Ch]
		push dword ptr[eax + 8h]
		push dword ptr[eax + 4h]
		push dword ptr[eax]
		call Trampoline
		mov result, eax
		jmp ResultAvailable

Trampoline:
		push ebp
		mov ebp, esp
		mov eax, s_addrMap
		add eax, 5
		jmp eax

ResultAvailable:
	}
	if (result != S_OK)
		return result;

#if TEXTURE_DUMPING_MODE
	if (pMappedResource != nullptr)
		s_mappedRes[pResource] = *pMappedResource;
#endif // TEXTURE_DUMPING_MODE

	if (pMappedResource != nullptr)
		s_suspectList.SetMappedData(pResource, *pMappedResource);

	return S_OK;
}


void WINAPI Unmap(
	[[maybe_unused]] ID3D11DeviceContext* pContext,
	[[maybe_unused]] ID3D11Resource* pResource,
	[[maybe_unused]] UINT Subresource
)
{
	s_suspectList.CollectGarbage();
	if (!s_suspectList.ActOn(pResource))
		s_suspectList.Remove(pResource);  // as mappedData will be invalid after Unmap(), we shouldn't keep it

#if TEXTURE_DUMPING_MODE
	DumpTexture(pContext, pResource, true);
#endif

	__asm {
		lea eax, pContext
		push dword ptr[eax + 8h]
		push dword ptr[eax + 4h]
		push dword ptr[eax]
		call Trampoline
		jmp ResultAvailable

Trampoline:
		push ebp
		mov ebp, esp
		mov eax, s_addrUnmap
		add eax, 5
		jmp eax

ResultAvailable:
	}
}


}  // unnamed namespace



namespace detour {



HRESULT WINAPI D3D11CreateDevice(
	IDXGIAdapter            *pAdapter,
	D3D_DRIVER_TYPE         DriverType,
	HMODULE                 Software,
	UINT                    Flags,
	CONST D3D_FEATURE_LEVEL *pFeatureLevels,
	UINT                    FeatureLevels,
	UINT                    SDKVersion,
	ID3D11Device            **ppDevice,
	D3D_FEATURE_LEVEL       *pFeatureLevel,
	ID3D11DeviceContext     **ppImmediateContext
)
{
	auto result = gan::Hook::GetTrampoline(::D3D11CreateDevice)(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);

	// hooking via vtable. ref: ID3D11DeviceVtbl and ID3D11DeviceContextVtbl in d3d11.h
	auto vtableDevice = *reinterpret_cast<void***>(*ppDevice);
	{
		s_addrCreateTexture2D = vtableDevice[5];
		gan::Hook hook { reinterpret_cast<decltype(CreateTexture2D)*>(s_addrCreateTexture2D), CreateTexture2D };
		hook.Install();
		DEBUG_MSG(L"s_addrCreateTexture2D = %p\n", s_addrCreateTexture2D);
	}
	auto vtableDeviceContext = *reinterpret_cast<void***>(*ppImmediateContext);
	{
		s_addrMap = vtableDeviceContext[14];
		gan::Hook hook { reinterpret_cast<decltype(Map)*>(s_addrMap), Map };
		hook.Install();
		DEBUG_MSG(L"s_addrMap = %p\n", s_addrMap);
	}
	{
		s_addrUnmap = vtableDeviceContext[15];
		gan::Hook hook { reinterpret_cast<decltype(Unmap)*>(s_addrUnmap), Unmap };
		hook.Install();
		DEBUG_MSG(L"s_addrUnmap = %p\n", s_addrUnmap);
	}

	return result;
}



}  // namespace detour

