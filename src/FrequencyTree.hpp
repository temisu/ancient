/* Copyright (C) Teemu Suutari */

#ifndef FREQUENCYTREE_HPP
#define FREQUENCYTREE_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <array>

// For exception
#include "Decompressor.hpp"

namespace ancient::internal
{

template<typename T,typename U,size_t V>
class FrequencyTree
{
public:
	FrequencyTree() noexcept
	{
		for (uint32_t i=0;i<_size;i++)
			_tree[i]=0;
	}

	~FrequencyTree()
	{
		// nothing needed
	}

	U decode(T value,T &low,T &freq) const
	{
		if (value>=_tree[_size-1])
			throw Decompressor::DecompressionError();
		U symbol=0;
		low=0;
		for (uint32_t i=_levels-2;;i--)
		{
			T tmp=_tree[_levelOffsets[i]+symbol];
			if (uint32_t(symbol+1)<_levelSizes[i] && value>=tmp)
			{
				symbol++;
				low+=tmp;
				value-=tmp;
			}
			if (!i) break;
			symbol<<=1;
		}
		freq=_tree[symbol];
		return symbol;
	}

	T operator[](U symbol) const noexcept
	{
		return _tree[symbol];
	}
	
	void add(U symbol,typename std::make_signed<T>::type freq) noexcept
	{
		for (uint32_t i=0;i<_levels;i++)
		{
			_tree[_levelOffsets[i]+symbol]+=freq;
			symbol>>=1;
		}
	}

	void set(U symbol,T freq) noexcept
	{
		// TODO: check behavior on large numbers
		typename std::make_signed<T>::type delta=freq-_tree[symbol];
		add(symbol,delta);
	}

	T getTotal() const noexcept
	{
		return _tree[_size-1];
	}

private:
	static constexpr uint32_t levelSize(uint32_t level)
	{
		uint32_t ret=V;
		for (uint32_t i=0;i<level;i++)
		{
			ret=(ret+1)>>1;
		}
		return ret;
	}

	static constexpr uint32_t levels()
	{
		uint32_t ret=0;
		while (levelSize(ret)!=1) ret++;
		return ret+1;
	}

	static constexpr uint32_t size()
	{
		uint32_t ret=0;
		for (uint32_t i=0;i<levels();i++)
			ret+=levelSize(i);
		return ret;
	}

	static constexpr uint32_t levelOffset(uint32_t level)
	{
		uint32_t ret=0;
		for (uint32_t i=0;i<level;i++)
			ret+=levelSize(i);
		return ret;
	}

	template<uint32_t... I>
	static constexpr auto makeLevelOffsetSequence(std::integer_sequence<uint32_t,I...>)
	{
		return std::integer_sequence<uint32_t,levelOffset(I)...>{};
	}

	template<uint32_t... I>
	static constexpr auto makeLevelSizeSequence(std::integer_sequence<uint32_t,I...>)
	{
		return std::integer_sequence<uint32_t,levelSize(I)...>{};
	}
 	
	template<uint32_t... I>
	static constexpr std::array<uint32_t,sizeof...(I)> makeArray(std::integer_sequence<uint32_t,I...>)
	{
		return std::array<uint32_t,sizeof...(I)>{{I...}};
	}

	static constexpr uint32_t			_size=size();
	static constexpr uint32_t			_levels=levels();
	static constexpr std::array<uint32_t,_levels>	_levelOffsets=makeArray(makeLevelOffsetSequence(std::make_integer_sequence<uint32_t,levels()>{}));
	static constexpr std::array<uint32_t,_levels>	_levelSizes=makeArray(makeLevelSizeSequence(std::make_integer_sequence<uint32_t,levels()>{}));

	T						_tree[size()];
};

}

#endif
