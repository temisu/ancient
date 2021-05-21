/* Copyright (C) Teemu Suutari */

#ifndef ANCIENT_BUFFER_HPP
#define ANCIENT_BUFFER_HPP

#ifndef ANCIENT_LIBRARY_VISIBILITY
#define ANCIENT_LIBRARY_VISIBILITY
#endif

#include <cstddef>
#include <cstdint>

#include <exception>

namespace ancient
{

class ANCIENT_LIBRARY_VISIBILITY Buffer
{
protected:
	Buffer() noexcept;

public:
	class ANCIENT_LIBRARY_VISIBILITY Error : public std::exception
	{
	public:
		Error() noexcept;
		virtual ~Error();
	};

	class ANCIENT_LIBRARY_VISIBILITY OutOfBoundsError : public Error
	{
	public:
		OutOfBoundsError() noexcept;
		virtual ~OutOfBoundsError();
	};

	class ANCIENT_LIBRARY_VISIBILITY InvalidOperationError : public Error
	{
	public:
		InvalidOperationError() noexcept;
		virtual ~InvalidOperationError();
	};

	class ANCIENT_LIBRARY_VISIBILITY OutOfMemoryError : public Error
	{
	public:
		OutOfMemoryError() noexcept;
		virtual ~OutOfMemoryError();
	};

	Buffer(const Buffer&)=delete;
	Buffer& operator=(const Buffer&)=delete;

	virtual ~Buffer();

	virtual const uint8_t *data() const noexcept=0;
	virtual uint8_t *data()=0;
	virtual size_t size() const noexcept=0;

	template<typename T>
	const T *cast() const noexcept
	{
		return reinterpret_cast<const T*>(data());
	}

	template<typename T>
	T *cast()
	{
		return reinterpret_cast<T*>(data());
	}

	virtual bool isResizable() const noexcept=0;
	virtual void resize(size_t newSize);

	uint8_t &operator[](size_t i);
	const uint8_t &operator[](size_t i) const;

	uint32_t readBE32(size_t offset) const;
	uint16_t readBE16(size_t offset) const;

	uint64_t readLE64(size_t offset) const;
	uint32_t readLE32(size_t offset) const;
	uint16_t readLE16(size_t offset) const;

	uint8_t read8(size_t offset) const;
};

}

#endif
