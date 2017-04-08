/* Copyright (C) Teemu Suutari */

#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <stddef.h>
#include <stdint.h>

class Buffer
{
protected:
	Buffer();

public:
	Buffer(const Buffer&)=delete;
	Buffer& operator=(const Buffer&)=delete;

	virtual ~Buffer();

	virtual const uint8_t *data() const=0;
	virtual uint8_t *data()=0;
	virtual size_t size() const=0;

	virtual bool isResizable() const;
	virtual void resize(size_t newSize);

	inline bool readBE(size_t offset,uint32_t &retValue) const
	{
		if (offset+4>size()) return false;
		const uint8_t *_ptr=reinterpret_cast<const uint8_t*>(data())+offset;
#if __BYTE_ORDER__ == 4321
		if (!(reinterpret_cast<size_t>(_ptr)&1))
		{
			retValue=*reinterpret_cast<const uint32_t*>(_ptr);
		} else
#elif __BYTE_ORDER__ == 1234
		// will fall through the else path
#else
#error
#endif
		{
			retValue=(uint32_t(_ptr[0])<<24)|(uint32_t(_ptr[1])<<16)|(uint32_t(_ptr[2])<<8)|uint32_t(_ptr[3]);
		}
		return true;
	}

	inline bool readBE(size_t offset,uint16_t &retValue) const
	{
		if (offset+2>size()) return false;
		const uint8_t *_ptr=reinterpret_cast<const uint8_t*>(data())+offset;
#if __BYTE_ORDER__ == 4321
		if (!(reinterpret_cast<size_t>(_ptr)&1))
		{
			retValue=*reinterpret_cast<const uint16_t*>(_ptr);
		} else
#elif __BYTE_ORDER__ == 1234
		// will fall through the else path
#else
#error
#endif
		{
			retValue=(uint16_t(_ptr[0])<<8)|uint16_t(_ptr[1]);
		}
		return true;
	}

	inline bool readLE(size_t offset,uint64_t &retValue) const
	{
		if (offset+8>size()) return false;
		const uint8_t *_ptr=reinterpret_cast<const uint8_t*>(data())+offset;
		retValue=(uint64_t(_ptr[7])<<56)|(uint64_t(_ptr[6])<<48)|(uint64_t(_ptr[5])<<40)|(uint64_t(_ptr[4])<<32)|
			(uint64_t(_ptr[3])<<24)|(uint64_t(_ptr[2])<<16)|(uint64_t(_ptr[1])<<8)|uint64_t(_ptr[0]);
		return true;
	}

	inline bool readLE(size_t offset,uint32_t &retValue) const
	{
		if (offset+4>size()) return false;
		const uint8_t *_ptr=reinterpret_cast<const uint8_t*>(data())+offset;
		retValue=(uint32_t(_ptr[3])<<24)|(uint32_t(_ptr[2])<<16)|(uint32_t(_ptr[1])<<8)|uint32_t(_ptr[0]);
		return true;
	}

	inline bool readLE(size_t offset,uint16_t &retValue) const
	{
		if (offset+2>size()) return false;
		const uint8_t *_ptr=reinterpret_cast<const uint8_t*>(data())+offset;
		retValue=(uint16_t(_ptr[1])<<8)|uint16_t(_ptr[0]);
		return true;
	}

	inline bool read(size_t offset,uint8_t &retValue) const
	{
		if (offset>=size()) return false;
		const uint8_t *_ptr=reinterpret_cast<const uint8_t*>(data())+offset;
		retValue=_ptr[0];
		return true;
	}
};

#endif
