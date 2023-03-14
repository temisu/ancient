/* Copyright (C) Teemu Suutari */

#include "common/Common.hpp"
#include "PPMQDecompressor.hpp"
#include "RangeDecoder.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "RangeDecoder.hpp"
#include "FrequencyTree.hpp"

#include <map>

namespace ancient::internal
{

bool PPMQDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("PPMQ");
}

std::shared_ptr<XPKDecompressor> PPMQDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::shared_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_shared<PPMQDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

PPMQDecompressor::PPMQDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::shared_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

PPMQDecompressor::~PPMQDecompressor()
{
	// nothing needed
}

const std::string &PPMQDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-PPMQ: PPM compressor";
	return name;
}

void PPMQDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	class BitReader : public RangeDecoder::BitReader
	{
	public:
		BitReader(ForwardInputStream &stream) noexcept :
			_reader(stream)
		{
			// nothing needed
		}

		virtual ~BitReader()
		{
			// nothing needed
		}

		virtual uint32_t readBit() override final
		{
			return _reader.readBitsBE32(1);
		}

		uint32_t readBits(uint32_t bitCount)
		{
			return _reader.readBitsBE32(bitCount);
		}

	private:
		MSBBitReader<ForwardInputStream>	_reader;
	};


	ForwardInputStream inputStream(_packedData,0,_packedData.size());
	ForwardOutputStream outputStream(rawData,0,rawData.size());
	BitReader bitReader(inputStream);

	for (uint32_t i=0;i<5;i++)
	{
		outputStream.writeByte(bitReader.readBits(8));
		// files shorter than 5 bytes are not supported by the library. We can try something here though
		if (outputStream.eof()) return;
	}

	RangeDecoder decoder(bitReader,bitReader.readBits(16));

	class ExclusionList
	{
	public:
		class ExclusionCallback
		{
		public:
			ExclusionCallback(ExclusionList &parent)
			{
				parent.registerCallback(this);
			}

			virtual void symbolIncluded(uint8_t symbol) noexcept=0;
			virtual void symbolExcluded(uint8_t symbol) noexcept=0;
		};

		ExclusionList() noexcept
		{
			for (uint32_t i=0;i<256U;i++)
				_tree.set(i,1);
		}

		~ExclusionList()
		{
			// nothing needed
		}

		void reset() noexcept
		{
			_tree.onNoMatch(1,[&](uint32_t i)
			{
				_tree.set(i,1);
				for (auto callback : _callbacks) callback->symbolIncluded(i);
			});
		}

		void exclude(uint8_t symbol) noexcept
		{
			if (_tree[symbol])
			{
				_tree.set(symbol,0);
				for (auto callback : _callbacks) callback->symbolExcluded(symbol);
			}
		}

		bool isExcluded(uint8_t symbol) const noexcept
		{
			return !_tree[symbol];
		}

		uint32_t getTotal() const noexcept
		{
			return _tree.getTotal();
		}

		uint8_t decode(uint16_t value,uint16_t &low,uint16_t &freq) const
		{
			return _tree.decode(value,low,freq);
		}

	private:
		void registerCallback(ExclusionCallback *callback) noexcept
		{
			_callbacks.push_back(callback);
		}

		FrequencyTree<uint16_t,uint8_t,256>	_tree;
		std::vector<ExclusionCallback*>		_callbacks;
	};

	class Model
	{
	public:
		Model(RangeDecoder &decoder,ExclusionList &exclusionList) noexcept :
			_decoder(decoder),
			_exclusionList(exclusionList)
		{
			// nothing needed
		}

		virtual ~Model()
		{
			// nothing needed
		}

		virtual bool decode(uint8_t &ch)=0;
		virtual void mark(uint8_t ch)=0;

	protected:
		RangeDecoder				&_decoder;
		ExclusionList				&_exclusionList;
	};

	class Model2 : public Model
	{
	public:
		Model2(RangeDecoder &decoder,ExclusionList &exclusionList) noexcept :
			Model(decoder,exclusionList)
		{
			// nothing needed
		}

		virtual ~Model2()
		{
			// nothing needed
		}

		virtual bool decode(uint8_t &ch) override final
		{
			return false;
		}

		virtual void mark(uint8_t ch) override final
		{
		}
	};

	class Model1 : public Model
	{
	public:
		Model1(RangeDecoder &decoder,ExclusionList &exclusionList) noexcept :
			Model(decoder,exclusionList)
		{
			// nothing needed
		}

		virtual ~Model1()
		{
			// nothing needed
		}

		virtual bool decode(uint8_t &ch) override final
		{
			return false;
		}

		virtual void mark(uint8_t ch) override final
		{
		}

	private:
		// Sparse frequency map with MFT, no good data type for this one
		// note that there is no sense to do MFT here. It is just something
		// the original implementation did
		// (maybe to optimize linked list traversing perhaps)
		struct Node
		{
			uint16_t			freq;
			uint8_t				symbol;
		};

		struct Context
		{
			uint16_t			escapeFreq;
			uint16_t			totalFreq;		// -escapeFreq
			std::vector<Node>		nodes;
		};

		uint32_t decode(Context &ctx)
		{
/*			uint16_t total=ctx.escapeFreq+ctx.totalFreq;
			uint16_t value=_decoder.decode(total);
			if (value.ctx<escapeFreq)
			{
				_decoder.scale(0,ctx.escapeFreq,total);
				return ~0U;
			}
			uint16_t low=escapeFreq;
			uint32_t size=ctx.nodes.size();
			for (uint32_t i=0;i<size;i++)
			{
				auto &node=ctx.nodes[i];
				if (low+node.freq<value)
				{
					_decoder.scale(low,low+node.freq,total);
					return i;
				}
				low+=node.freq;
			}*/
			throw Decompressor::DecompressionError();
		}


		std::map<std::pair<uint16_t,uint32_t>,Context> _contexts;
	};

	// A simple arithmetic encoder
	class Model0 : public Model, public ExclusionList::ExclusionCallback
	{
	public:
		Model0(RangeDecoder &decoder,ExclusionList &exclusionList) noexcept :
			Model(decoder,exclusionList),
			ExclusionCallback(exclusionList)
		{
			_tree.set(0,1);
			for (uint32_t i=0;i<256U;i++) _charCounts[i]=0;
		}

		virtual ~Model0()
		{
			// nothing needed
		}

		virtual bool decode(uint8_t &ch) override final
		{
			uint16_t value=_decoder.decode(_tree.getTotal());
			uint16_t low,freq;
			uint8_t symbol=_tree.decode(value,low,freq);
			_decoder.scale(low,low+freq,_tree.getTotal());
			if (!symbol)
			{
				_tree.onNoMatch(0,[&](uint32_t i)
				{
					if (i) _exclusionList.exclude(i-1);
				});
				return false;
			}
			_tree.add(symbol,1);
			if (_tree[symbol]==2 && _tree[0]!=1U) _tree.add(0,-1);
			ch=symbol-1;
			return true;
		}
		
		virtual void mark(uint8_t ch) override final
		{
			uint16_t symbol=uint16_t(ch)+1;
			if (!_tree[symbol])		// TODO: if looks like always true!
			{
				if (_exclusionList.isExcluded(ch)) _charCounts[ch]=1U;	// TODO: looks like always false
					else _tree.set(symbol,1U);
				_tree.add(0,1);
			}
		}

		virtual void symbolIncluded(uint8_t symbol) noexcept override final
		{
			_tree.set(uint16_t(symbol)+1,_charCounts[symbol]);
		}

		virtual void symbolExcluded(uint8_t symbol) noexcept override final
		{
			_charCounts[symbol]=_tree[symbol];
			_tree.set(uint16_t(symbol)+1,0);
		}

	private:
		// first one is escape character
		FrequencyTree<uint16_t,uint16_t,257>	_tree;
		uint16_t				_charCounts[256];
	};

	// purely a fallback model that can always decode
	class FallbackModel : public Model
	{
	public:
		FallbackModel(RangeDecoder &decoder,ExclusionList &exclusionList) noexcept :
			Model(decoder,exclusionList)
		{
			// nothing needed
		}

		virtual ~FallbackModel()
		{
			// nothing needed
		}

		virtual bool decode(uint8_t &ch) override final
		{
			uint16_t value=_decoder.decode(_exclusionList.getTotal());
			uint16_t low,freq;
			ch=_exclusionList.decode(value,low,freq);
			_decoder.scale(low,low+freq,_exclusionList.getTotal());
			return true;
		}

		virtual void mark(uint8_t ch) override final
		{
			// nothing needed
		}
	};

	ExclusionList exclusionList;
	Model2 model2A(decoder,exclusionList);
	Model2 model2B(decoder,exclusionList);
	Model1 model1A(decoder,exclusionList);
	Model1 model1B(decoder,exclusionList);
	Model1 model1C(decoder,exclusionList);
	Model0 model0(decoder,exclusionList);
	FallbackModel fallbackModel(decoder,exclusionList);
	std::vector<Model*> models={&model2A,&model2B,&model1A,&model1B,&model1C,&model0,&fallbackModel};

	// unsurprisingly this fails for short blocks!
	for (uint32_t i=0;i<5U;i++)
		outputStream.writeByte(bitReader.readBits(8U));

	while (!outputStream.eof())
	{
		exclusionList.reset();
		for (uint32_t i=0;i<models.size();i++)
		{
			uint8_t ch;
			if (models[i]->decode(ch))
			{
				for (uint32_t j=i;j;j--)
					models[j-1]->mark(ch);
				outputStream.writeByte(ch);
				break;
			}
		}
	}
}

}
