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

	inline bool read(size_t offset,uint32_t &retValue) const
	{
		if (offset+4>size()) return false;
		const uint8_t *_ptr=reinterpret_cast<const uint8_t*>(data())+offset;
		retValue=(uint32_t(_ptr[0])<<24)|(uint32_t(_ptr[1])<<16)|(uint32_t(_ptr[2])<<8)|uint32_t(_ptr[3]);
		return true;
	}

	inline bool read(size_t offset,uint16_t &retValue) const
	{
		if (offset+2>size()) return false;
		const uint8_t *_ptr=reinterpret_cast<const uint8_t*>(data())+offset;
		retValue=(uint16_t(_ptr[0])<<8)|uint16_t(_ptr[1]);
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

// helpers to splice Buffer

template <typename T>
class GenericSubBuffer : public Buffer
{
public:
	GenericSubBuffer(const GenericSubBuffer&)=delete;
	GenericSubBuffer& operator=(const GenericSubBuffer&)=delete;

	GenericSubBuffer(T &base,size_t start,size_t length) :
		_base(base),
		_start(start),
		_length(length)
	{
		// if the sub-buffer is invalid, we set both _start and _length to 0
		if (start+length>_base.size())
		{
			_start=0;
			_length=0;
		}
	}
	
	virtual ~GenericSubBuffer() { }

	virtual const uint8_t *data() const override
	{
		return _base.data()+_start;
	}

	virtual uint8_t *data() override;

	virtual size_t size() const override
	{
		return _length;
	}

	virtual bool isResizable() const override
	{
		return false;
	}

	virtual void resize(size_t newSize) override { }

private:
	T &_base;
	size_t	_start;
	size_t	_length;
};

typedef GenericSubBuffer<Buffer> SubBuffer;
typedef GenericSubBuffer<const Buffer> ConstSubBuffer;

#endif
