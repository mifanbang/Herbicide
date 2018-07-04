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

#include "d3d11.h"

#if TEXTURE_DUMPING_MODE
	#include <D3DX11tex.h>
#endif  // TEXTURE_DUMPING_MODE

#include <gandr/hooking.h>

#include "shared/util.h"
#include "../TextureFilter.h"


namespace {


	
uint32_t sOffsetCreateTexture2D = 0;
uint32_t sOffsetUnmap = 0;
uint32_t sOffsetMap = 0;

ResourceSuspectList s_suspectList;

#if TEXTURE_DUMPING_MODE
std::unordered_map<ID3D11Resource*, D3D11_MAPPED_SUBRESOURCE> s_mappedRes;
#endif  // TEXTURE_DUMPING_MODE


}  // unnamed namespace



namespace detour {



HRESULT WINAPI CreateTexture2D(
	[[maybe_unused]]ID3D11Device* pDevice,
	[[maybe_unused]]const D3D11_TEXTURE2D_DESC* pDesc,
	[[maybe_unused]]const D3D11_SUBRESOURCE_DATA* pInitialData,
	[[maybe_unused]]ID3D11Texture2D** ppTexture2D
)
{
	HRESULT result;
	__asm {
		push dword ptr[ebp + 14h]
		push dword ptr[ebp + 10h]
		push dword ptr[ebp + 0Ch]
		push dword ptr[ebp + 8h]
		call Trampoline
		mov result, eax
		jmp ResultAvailable

Trampoline:
		push ebp
		mov ebp, esp
		mov eax, D3D11CreateDevice
		add eax, sOffsetCreateTexture2D
		add eax, 5
		jmp eax

ResultAvailable:
	}

	DEBUG_MSG(L"CreateTexture2D w=%d h=%d fmt=%d usg=%d dat=%p tex=%p %08X\n", pDesc->Width, pDesc->Height, pDesc->Format, pDesc->Usage, pInitialData, ppTexture2D ? *ppTexture2D : nullptr, GetTickCount());

	if (result != S_OK)
		return result;

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
	HRESULT result;
	__asm {
		push dword ptr[ebp + 1Ch]
		push dword ptr[ebp + 18h]
		push dword ptr[ebp + 14h]
		push dword ptr[ebp + 10h]
		push dword ptr[ebp + 0Ch]
		push dword ptr[ebp + 8h]
		call Trampoline
		mov result, eax
		jmp ResultAvailable

Trampoline:
		push ebp
		mov ebp, esp
		mov eax, D3D11CreateDevice
		add eax, sOffsetMap
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
	static bool callFlag = false;
	D3D11_RESOURCE_DIMENSION type;
	pResource->GetType(&type);
	if (!callFlag && type == ::D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
		D3D11_TEXTURE2D_DESC desc;
		reinterpret_cast<::ID3D11Texture2D*>(pResource)->GetDesc(&desc);

		gan::Hash<256> hash;
		{
			auto itr = s_mappedRes.find(pResource);
			if (itr == s_mappedRes.cend())
				goto EndDumpTexture;
			auto mapped = itr->second;
			s_mappedRes.erase(pResource);

			if (desc.Height >= 256)
				gan::Hasher::GetSHA(mapped.pData, mapped.RowPitch << 8, hash);
			else
				goto EndDumpTexture;
		}

		std::string path;
		{
			path.reserve(MAX_PATH);
			path.append("tx\\tx_");
			for (int i = 0; i < 32; ++i) {
				char buf[32];
				sprintf_s(buf, sizeof(buf), "%02X", hash.data[i]);
				path.append(buf);
			}
			path.push_back('_');
			path.append(std::to_string(desc.Format));
			path.append(".png");
		}

		callFlag = true;
		::D3DX11SaveTextureToFileA(pContext, pResource, ::D3DX11_IFF_PNG, path.c_str());
		callFlag = false;
	}
EndDumpTexture:
#endif  // TEXTURE_DUMPING_MODE

	__asm {
		push dword ptr[ebp + 10h]
		push dword ptr[ebp + 0Ch]
		push dword ptr[ebp + 8h]
		call Trampoline
		jmp ResultAvailable

Trampoline:
		push ebp
		mov ebp, esp
		mov eax, D3D11CreateDevice
		add eax, sOffsetUnmap
		add eax, 5
		jmp eax

ResultAvailable:
	}
}


HRESULT WINAPI MyD3D11CreateDevice(
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
	auto result = CallTram32(::D3D11CreateDevice)(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);

	// hooking via vtable
	auto vtableDevice = *reinterpret_cast<uint32_t**>(*ppDevice);
	{
		sOffsetCreateTexture2D = vtableDevice[5] - reinterpret_cast<uint32_t>(D3D11CreateDevice);
		gan::InlineHooking32 hook(reinterpret_cast<decltype(detour::CreateTexture2D)*>(vtableDevice[5]), detour::CreateTexture2D);
		hook.Hook();
	}
	auto vtableDeviceContext = *reinterpret_cast<uint32_t**>(*ppImmediateContext);
	{
		sOffsetMap = vtableDeviceContext[14] - reinterpret_cast<uint32_t>(D3D11CreateDevice);
		gan::InlineHooking32 hook(reinterpret_cast<decltype(detour::Map)*>(vtableDeviceContext[14]), detour::Map);
		hook.Hook();
	}
	{
		sOffsetUnmap = vtableDeviceContext[15] - reinterpret_cast<uint32_t>(D3D11CreateDevice);
		gan::InlineHooking32 hook(reinterpret_cast<decltype(detour::Unmap)*>(vtableDeviceContext[15]), detour::Unmap);
		hook.Hook();
	}

	return result;
}



}  // namespace detour

