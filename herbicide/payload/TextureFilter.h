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

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#pragma warning(push)
#pragma warning(disable: 4005)  // macro redefinition
#include <d3d11.h>
#pragma warning(pop)
#include <windows.h>



using FilterDescCondition = std::function<bool (const D3D11_TEXTURE2D_DESC&)>;
using FilterDataCondition = std::function<bool (const D3D11_MAPPED_SUBRESOURCE&)>;
using FilterDataAction = std::function<bool (const D3D11_MAPPED_SUBRESOURCE&)>;



class DataFilter
{
public:
	DataFilter(const FilterDataCondition& condition, const FilterDataAction& action);
	bool ActUponMappedData(const D3D11_MAPPED_SUBRESOURCE& data);


private:
	FilterDataCondition m_condition;
	FilterDataAction m_action;
};

using DataFilterList = std::vector<std::shared_ptr<DataFilter>>;



class DataFilterFactory
{
public:
	struct Entry
	{
		FilterDescCondition descCond;
		FilterDataCondition dataCond;
		FilterDataAction action;
	};


	DataFilterFactory();

	void Register(const Entry& entry);
	bool Match(const D3D11_TEXTURE2D_DESC& desc, DataFilterList& out);
	size_t GetEntryCount() const;


private:
	std::vector<std::pair<FilterDescCondition, std::shared_ptr<DataFilter>>> m_registry;
};



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


constexpr unsigned int cSuspectTimeOutSec = 3;
using ResourceSuspect = TimedResourceSuspect<cSuspectTimeOutSec>;


class ResourceSuspectList : private std::unordered_map<void*, ResourceSuspect>
{
	using super = std::unordered_map<void*, ResourceSuspect>;

public:
	ResourceSuspectList();

	void Add(void* ptr, ResourceSuspect&& suspect);
	void Remove(void* ptr);
	void SetMappedData(void* ptr, const D3D11_MAPPED_SUBRESOURCE& data);
	bool ActOn(void* ptr);  // does not check timestamp
	void CollectGarbage();
};



DataFilterFactory& GetDataFilterFactory();

