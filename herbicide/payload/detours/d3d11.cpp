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

#define NOMINMAX
#define TEXTURE_DUMPING_MODE	0

#include "d3d11.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>

#if TEXTURE_DUMPING_MODE
	#include <D3DX11tex.h>
#endif  // TEXTURE_DUMPING_MODE

#include <gandr/hooking.h>

#include "shared/util.h"



namespace {



constexpr unsigned int cSuspectTimeOutSec = 3;

using FilterDescCondition = std::function<bool (const D3D11_TEXTURE2D_DESC&)>;
using FilterDataCondition = std::function<bool (const D3D11_MAPPED_SUBRESOURCE&)>;
using FilterDataAction = std::function<bool (const D3D11_MAPPED_SUBRESOURCE&)>;


class DataFilter
{
public:
	DataFilter(const FilterDataCondition& condition, const FilterDataAction& action)
		: m_condition(condition)
		, m_action(action)
	{
	}

	bool ActUponMappedData(const D3D11_MAPPED_SUBRESOURCE& data)
	{
		if (!m_condition(data))
			return false;
		return m_action(data);
	}

private:
	FilterDataCondition m_condition;
	FilterDataAction m_action;
};

using DataFilterList = std::vector<std::shared_ptr<DataFilter>>;



// suspect of the resource we are looking for, attached with a list of conditions to check and actions to take
template <unsigned int TimeOutSec>
struct TimedResourceSuspect
{
	uint64_t timestamp;
	D3D11_MAPPED_SUBRESOURCE mappedData;
	DataFilterList filterList;


	TimedResourceSuspect()
		: timestamp(GetTickCount64())
		, mappedData()
		, filterList()
	{
		mappedData.pData = nullptr;
	}

	bool IsTimedOut() const
	{
		return (GetTickCount64() - timestamp) >= static_cast<uint64_t>(TimeOutSec * 1000);
	}

	void RenewTimeStamp()
	{
		timestamp = GetTickCount64();
	}

	bool IsDataReady() const
	{
		return mappedData.pData != nullptr;
	}
};

using ResourceSuspect = TimedResourceSuspect<cSuspectTimeOutSec>;


class ResourceSuspectList : private std::unordered_map<void*, ResourceSuspect>
{
	using super = std::unordered_map<void*, ResourceSuspect>;

public:
	ResourceSuspectList()
		: super()
	{
	}

	void Add(void* ptr, ResourceSuspect&& suspect)
	{
		super::emplace(ptr, suspect);
	}

	void Remove(void* ptr)
	{
		super::erase(ptr);
	}

	void SetMappedData(void* ptr, const D3D11_MAPPED_SUBRESOURCE& data)
	{
		auto itr = super::find(ptr);
		if (itr != super::cend()) {
			itr->second.mappedData = data;
			itr->second.RenewTimeStamp();
		}
	}

	// does not check timestamp
	bool ActOn(void* ptr)
	{
		bool hasActionTaken = false;
		auto itr = super::find(ptr);
		if (itr != super::cend() && itr->second.IsDataReady()) {
			const auto& mappedData = itr->second.mappedData;
			for (auto& filter : itr->second.filterList)
				hasActionTaken = hasActionTaken || filter->ActUponMappedData(mappedData);
			if (hasActionTaken)
				super::erase(itr);
		}
		return hasActionTaken;
	}

	void CollectGarbage()
	{
		std::vector<void*> eraseList;
		for (auto& suspect : *this) {
			if (suspect.second.IsTimedOut())
				eraseList.emplace_back(suspect.first);
		}
		for (auto item : eraseList)
			super::erase(item);
	}
};



class DataFilterFactory
{
public:
	struct Entry
	{
		FilterDescCondition descCond;
		FilterDataCondition dataCond;
		FilterDataAction action;
	};


	DataFilterFactory()
		: m_registry()
	{
	}

	void Register(const Entry& entry)
	{
		m_registry.emplace_back(entry.descCond, std::make_shared<DataFilter>(entry.dataCond, entry.action));
	}

	bool Match(const D3D11_TEXTURE2D_DESC& desc, DataFilterList& out)
	{
		DataFilterList result;
		for (const auto& item : m_registry) {
			if (item.first(desc))
				result.push_back(item.second);
		}
		std::swap(out, result);
		return out.size() > 0;
	}

	size_t GetEntryCount() const
	{
		return m_registry.size();
	}


private:
	std::vector<std::pair<FilterDescCondition, std::shared_ptr<DataFilter>>> m_registry;
};



bool MatchHash(const void* data, unsigned int size, const gan::Hash<256>& hash)
{
	gan::Hash<256> hashOther;
	if (gan::Hasher::GetSHA(data, size, hashOther) != NO_ERROR)
		return false;
	return hash == hashOther;
}


void ErasePixels(const D3D11_MAPPED_SUBRESOURCE& data, const D3D11_RECT& rect, uint8_t stride)
{
	const unsigned int offsetOfCol = rect.top * data.RowPitch;
	const unsigned int offsetInRow = rect.left * stride;
	const unsigned int numErasedBytesPerRow = (rect.right - rect.left + 1) * stride;

	LONG i = rect.top;
	auto dataPtr = reinterpret_cast<uint8_t*>(data.pData) + offsetOfCol + offsetInRow;
	for (; i <= rect.bottom; ++i, dataPtr += data.RowPitch)
		memset(dataPtr, 0x00, numErasedBytesPerRow);
}


void SetupDataFilterFactory(DataFilterFactory& factory)
{
#define MAKE_DESC_FILTER(w, h, format) \
	( [](const D3D11_TEXTURE2D_DESC& desc) -> bool { \
		return desc.Width == w && desc.Height == h && desc.Format == format && desc.Usage == D3D11_USAGE_STAGING; \
	} )

// function checks first 256 rows
#define MAKE_DATA_FILTER(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11,h12,h13,h14,h15,h16,h17,h18,h19,h20,h21,h22,h23,h24,h25,h26,h27,h28,h29,h30,h31) \
	( [](const D3D11_MAPPED_SUBRESOURCE& data) -> bool { \
		return MatchHash( \
			data.pData, \
			data.RowPitch << 8, \
			{ h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11,h12,h13,h14,h15,h16,h17,h18,h19,h20,h21,h22,h23,h24,h25,h26,h27,h28,h29,h30,h31 } \
		); \
	 } )

#define MAKE_DATA_ERASER(x, y, w, h, stride) \
	( [](const D3D11_MAPPED_SUBRESOURCE& data) -> bool { \
		ErasePixels(data, {x, y, x+w-1, y+h-1}, stride); \
		return true; \
	} )


	// Dark Elf: game (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x04,0xF1,0xB2,0x4E,0x5D,0x9A,0xB0,0x1C,0xA3,0xC6,0x87,0x95,0x17,0x8E,0x98,0x63,0xD6,0x26,0x8D,0xBA,0x6E,0xEA,0xE5,0xB1,0xA6,0xDE,0x2E,0xA6,0x90,0xCA,0x51,0xD1),
		MAKE_DATA_ERASER(1733, 1721, 56, 56, 4)
	});
	// Dark Elf: ecchi scene (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x92,0x7D,0x61,0xC6,0xD5,0x1A,0xE6,0x37,0x9A,0x76,0xA9,0x93,0x55,0x62,0xEA,0x0D,0x31,0x53,0xB7,0x3E,0xC1,0x1B,0xD6,0xFA,0x25,0x6A,0x58,0xF7,0xC5,0x79,0x8A,0x0B),
		MAKE_DATA_ERASER(1687, 1992, 56, 56, 4)
	});
	// Dark Elf: ecchi scene (rabbit)
	factory.Register({
		MAKE_DESC_FILTER(2048, 256, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x59,0x77,0x24,0x9A,0x4F,0x07,0xFA,0x5B,0x4A,0x41,0x3A,0x3F,0x92,0x9C,0x2A,0xCC,0x8A,0xEC,0xE7,0xD4,0x65,0x2F,0xB7,0x8A,0xDD,0x22,0x12,0x7E,0x44,0x00,0x5E,0x78),
		MAKE_DATA_ERASER(577, 0, 183, 256, 4)
	});

#undef MAKE_DESC_FILTER
#undef MAKE_DATA_FILTER
#undef MAKE_DATA_ERASER
}


DataFilterFactory& GetDataFilterFactory()
{
	static DataFilterFactory factory;
	if (factory.GetEntryCount() == 0)
		SetupDataFilterFactory(factory);
	return factory;
}


ResourceSuspectList s_suspectList;


}  // unnamed namespace



namespace detour {



static uint32_t sOffsetCreateTexture2D = 0;
static uint32_t sOffsetUnmap = 0;
static uint32_t sOffsetMap = 0;

#if TEXTURE_DUMPING_MODE
static std::unordered_map<ID3D11Resource*, D3D11_MAPPED_SUBRESOURCE> s_mappedRes;
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

