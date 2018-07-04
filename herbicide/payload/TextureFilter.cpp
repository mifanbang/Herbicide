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


	// Witch Girl: battle (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x66,0xFD,0x9D,0x8D,0x37,0x33,0xA9,0xA7,0x63,0xFC,0xF7,0x96,0x0B,0xFF,0x58,0x06,0x11,0x7E,0x7A,0x6B,0xB6,0xE1,0x40,0xE7,0x4B,0x8C,0x47,0xFF,0x9F,0xD0,0x2F,0x33),
		MAKE_DATA_ERASER(135, 955, 56, 56, 4)
	});
	// Witch Girl: ecchi scene (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x2A,0x0B,0xA7,0xE5,0x90,0x42,0x99,0x94,0xB0,0xFF,0x6E,0xF3,0x08,0x4F,0xF5,0xDE,0xD2,0xFC,0xB4,0x7E,0x80,0xA1,0x3B,0x9F,0x75,0x78,0x06,0x1C,0x3A,0x77,0xA2,0x32),
		MAKE_DATA_ERASER(1211, 680, 56, 56, 4)
	});
	// Witch Girl: ecchi scene (rabbit)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x2A,0x0B,0xA7,0xE5,0x90,0x42,0x99,0x94,0xB0,0xFF,0x6E,0xF3,0x08,0x4F,0xF5,0xDE,0xD2,0xFC,0xB4,0x7E,0x80,0xA1,0x3B,0x9F,0x75,0x78,0x06,0x1C,0x3A,0x77,0xA2,0x32),
		MAKE_DATA_ERASER(1560, 440, 155, 184, 4)
	});


	// Zombie Girl: battle (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x7C,0xD2,0xB0,0x58,0x9B,0x2C,0xAF,0x59,0x44,0x27,0x9B,0xDD,0xB3,0xED,0xCC,0x71,0x7D,0x9B,0x4D,0xBB,0xB1,0x20,0x3E,0xD1,0x02,0x49,0xFE,0x86,0x7E,0x19,0x1B,0x09),
		MAKE_DATA_ERASER(963, 1054, 56, 56, 4)
	});
	// Zombie Girl does not have an uncensorable flower in the ecchi scene
	// Zombie Girl: ecchi scene (rabbit)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0xB5,0xF7,0xC3,0x4A,0xB5,0x6F,0x0C,0x1A,0x54,0x09,0x63,0xB7,0x6F,0x49,0x7A,0x2F,0xB0,0xD5,0x53,0x86,0x64,0x96,0x36,0xCF,0xB1,0xF7,0x33,0xF3,0xDA,0x3E,0x52,0x13),
		MAKE_DATA_ERASER(1517, 1220, 184, 154, 4)
	});


	// Dragon Maiden: battle (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0xAE,0x15,0xDE,0x83,0xAA,0x71,0x9C,0x37,0xBA,0x69,0x38,0x55,0x95,0x1E,0x2F,0x9C,0xE3,0x4E,0xE8,0x75,0x22,0x06,0xAF,0xCF,0x3A,0x66,0x61,0xC1,0x4C,0x90,0xA6,0x23),
		MAKE_DATA_ERASER(0, 1995, 54, 53, 4)
	});
	// Dragon Maiden does not have a flower in the ecchi scene
	// Dragon Maiden: ecchi scene (rabbit)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x56,0xF7,0xE6,0x07,0xDA,0xE2,0xEB,0x1C,0x4D,0xBE,0x0A,0xF5,0xDF,0x36,0x58,0x2D,0x34,0x54,0x99,0x51,0x85,0x0C,0x0C,0x60,0xF7,0x27,0xC9,0x56,0x4C,0x49,0x7B,0x24),
		MAKE_DATA_ERASER(1628, 309, 184, 155, 4)
	});


	// Beast Girl: battle (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x42,0xE1,0xAB,0xED,0xC5,0xF7,0x30,0xB9,0xE9,0xB1,0xB6,0x57,0xAA,0x07,0x86,0x20,0x67,0xD5,0xB4,0xEC,0x0B,0xC1,0x6B,0x58,0xF3,0xE5,0x4E,0xD4,0xDC,0xE8,0x0E,0x65),
		MAKE_DATA_ERASER(1296, 1988, 55, 54, 4)
	});
	// Beast Girl: ecchi scene (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x69,0xDB,0xA2,0x6D,0x4A,0x2B,0x21,0x6A,0x9A,0xF2,0xB0,0xDF,0x5A,0x4D,0xE5,0xBD,0x81,0x8F,0x89,0x9C,0x62,0x7B,0xB7,0xFB,0xED,0xEE,0x41,0x63,0xB6,0x39,0xF3,0xE0),
		MAKE_DATA_ERASER(1992, 830, 54, 55, 4)
	});
	// Beast Girl: ecchi scene (rabbit)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x69,0xDB,0xA2,0x6D,0x4A,0x2B,0x21,0x6A,0x9A,0xF2,0xB0,0xDF,0x5A,0x4D,0xE5,0xBD,0x81,0x8F,0x89,0x9C,0x62,0x7B,0xB7,0xFB,0xED,0xEE,0x41,0x63,0xB6,0x39,0xF3,0xE0),
		MAKE_DATA_ERASER(1602, 1880, 184, 155, 4)
	});


	// Pharaoh: battle (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0xC1,0x9D,0xA0,0x00,0x27,0x7C,0x42,0x5B,0x15,0x70,0x94,0x9D,0x24,0x80,0x16,0xDC,0xA4,0x4D,0x0F,0x0D,0xFE,0xB0,0x9C,0x6D,0x90,0x51,0x9C,0xB2,0x26,0x9F,0x21,0xC1),
		MAKE_DATA_ERASER(1990, 977, 55, 53, 4)
	});
	// Pharaoh: ecchi scene (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x15,0xEE,0xB4,0x6F,0xDA,0x63,0x50,0xF9,0x96,0x84,0x1E,0xC1,0x5A,0x18,0x62,0xD0,0x67,0x02,0x73,0x85,0xF3,0x91,0xAD,0x44,0x8B,0x12,0xD1,0x69,0xDE,0x11,0x79,0xF4),
		MAKE_DATA_ERASER(1155, 715, 54, 55, 4)
	});
	// Pharaoh: ecchi scene (rabbit)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x95,0xF8,0xD9,0xC7,0x55,0x31,0x07,0x84,0x28,0xCE,0xA6,0xA1,0xE2,0xF9,0x93,0x25,0x97,0xCA,0x46,0x6E,0x17,0x24,0xEE,0x1E,0x70,0x79,0xC0,0xD4,0xBF,0x71,0x15,0x9E),
		MAKE_DATA_ERASER(944, 1637, 155, 185, 4)
	});


	// Warrior Girl: battle (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x8D,0x12,0xBC,0x04,0x5C,0x92,0xC0,0x40,0xBA,0x48,0xE9,0x63,0xD6,0xDD,0xAC,0x43,0x67,0x65,0x5B,0x39,0x4C,0x5C,0x68,0xAB,0xE5,0x17,0x16,0x45,0xCB,0x3C,0x54,0xE9),
		MAKE_DATA_ERASER(1978, 256, 55, 54, 4)
	});
	// Warrior Girl: ecchi scene (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x2C,0x04,0xC8,0x4B,0x46,0x7F,0x6E,0x3C,0x80,0x77,0x39,0x47,0x85,0x60,0x5F,0x5A,0xD2,0x99,0x74,0x2E,0xAF,0xB9,0xAE,0x18,0x4E,0x23,0xC1,0x47,0xE0,0x6A,0xC3,0xE5),
		MAKE_DATA_ERASER(1989, 15, 55, 54, 4)
	});
	// Warrior Girl: ecchi scene (rabbit)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x5A,0x5B,0x66,0x3F,0x76,0x43,0x0A,0x9A,0xE7,0x7B,0xA4,0xDD,0x2B,0x1D,0x08,0x5B,0xCA,0xB0,0x78,0xCA,0xA1,0xC9,0xCF,0x1E,0xDD,0x78,0xC7,0xEF,0x33,0x62,0x61,0xBB),
		MAKE_DATA_ERASER(1853, 1438, 155, 183, 4)
	});


	// Preist: battle (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0x5D,0x26,0x38,0x93,0xAF,0xCB,0x1C,0xD3,0x27,0x47,0xCD,0x18,0x41,0xE3,0x14,0xD6,0x89,0x0A,0xE0,0x87,0x2A,0x76,0x71,0x65,0x37,0xEE,0xB1,0x2A,0x5B,0xE2,0x6D,0x0A),
		MAKE_DATA_ERASER(748, 1706, 54, 55, 4)
	});
	// Preist: ecchi scene (flower)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0xD3,0xAC,0x08,0x3F,0x28,0xF0,0x96,0x02,0xC0,0xD4,0x97,0xE7,0x56,0x8D,0xE0,0x56,0x75,0x19,0xFA,0x7C,0x41,0x2A,0xD4,0x22,0x7F,0x04,0x2E,0x5E,0x5A,0xC3,0x37,0xBF),
		MAKE_DATA_ERASER(1845, 1226, 54, 55, 4)
	});
	// Preist: ecchi scene (rabbit)
	factory.Register({
		MAKE_DESC_FILTER(2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM),
		MAKE_DATA_FILTER(0xD3,0xAC,0x08,0x3F,0x28,0xF0,0x96,0x02,0xC0,0xD4,0x97,0xE7,0x56,0x8D,0xE0,0x56,0x75,0x19,0xFA,0x7C,0x41,0x2A,0xD4,0x22,0x7F,0x04,0x2E,0x5E,0x5A,0xC3,0x37,0xBF),
		MAKE_DATA_ERASER(1691, 1097, 155, 184, 4)
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


