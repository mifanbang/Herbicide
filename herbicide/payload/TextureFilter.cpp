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

#include "TextureFilter.h"

#include <gandr/Hash.h>



namespace {



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


	// Dark Elf: battle (flower)
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



}  // unnamed namespace



DataFilter::DataFilter(const FilterDataCondition& condition, const FilterDataAction& action)
	: m_condition(condition)
	, m_action(action)
{
}


bool DataFilter::ActUponMappedData(const D3D11_MAPPED_SUBRESOURCE& data)
{
	if (!m_condition(data))
		return false;
	return m_action(data);
}



DataFilterFactory::DataFilterFactory()
	: m_registry()
{
}


void DataFilterFactory::Register(const Entry& entry)
{
	m_registry.emplace_back(entry.descCond, std::make_shared<DataFilter>(entry.dataCond, entry.action));
}


bool DataFilterFactory::Match(const D3D11_TEXTURE2D_DESC& desc, DataFilterList& out)
{
	DataFilterList result;
	for (const auto& item : m_registry) {
		if (item.first(desc))
			result.push_back(item.second);
	}
	std::swap(out, result);
	return out.size() > 0;
}


size_t DataFilterFactory::GetEntryCount() const
{
	return m_registry.size();
}



ResourceSuspectList::ResourceSuspectList()
	: super()
{
}


void ResourceSuspectList::Add(void* ptr, ResourceSuspect&& suspect)
{
	super::emplace(ptr, suspect);
}


void ResourceSuspectList::Remove(void* ptr)
{
	super::erase(ptr);
}


void ResourceSuspectList::SetMappedData(void* ptr, const D3D11_MAPPED_SUBRESOURCE& data)
{
	auto itr = super::find(ptr);
	if (itr != super::cend()) {
		itr->second.mappedData = data;
		itr->second.RenewTimeStamp();
	}
}


// does not check timestamp
bool ResourceSuspectList::ActOn(void* ptr)
{
	bool hasActionTaken = false;
	auto itr = super::find(ptr);
	if (itr != super::cend() && itr->second.IsDataReady()) {
		const auto& mappedData = itr->second.mappedData;
		for (auto& filter : itr->second.filterList)
			hasActionTaken = filter->ActUponMappedData(mappedData) || hasActionTaken;
		if (hasActionTaken)
			super::erase(itr);
	}
	return hasActionTaken;
}


void ResourceSuspectList::CollectGarbage()
{
	std::vector<void*> eraseList;
	for (auto& suspect : *this) {
		if (suspect.second.IsTimedOut())
			eraseList.emplace_back(suspect.first);
	}
	for (auto item : eraseList)
		super::erase(item);
}



DataFilterFactory& GetDataFilterFactory()
{
	static DataFilterFactory factory;
	if (factory.GetEntryCount() == 0)
		SetupDataFilterFactory(factory);
	return factory;
}


ResourceSuspectList s_suspectList;


