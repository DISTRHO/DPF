/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_RING_BUFFER_HPP_INCLUDED
#define DISTRHO_RING_BUFFER_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Buffer structs

/**
   Base structure for all RingBuffer containers.
   This struct details the data model used in DPF's RingBuffer class.

   DPF RingBuffer uses a struct just like this one to store positions, buffer data, size, etc.
   The RingBuffer itself takes ownership of this struct and uses it to store any needed data.
   This allows to dynamically change the way its ring buffer is allocated, simply by changing the template type.
   For example, `RingBufferControl<HeapBuffer>` will create a ring buffer with heap memory, which can be of any size.
   In the same vein, `RingBufferControl<SmallStackBuffer>` will create a ring buffer with stack memory,
   directly tied to the RingBufferControl it belongs to.

   The main idea behind this model is to allow RingBufferControl over memory created elsewhere,
   for example shared memory area.
   One can create/place the Buffer struct in shared memory, and point RingBufferControl to it,
   thus avoiding the pitfalls of sharing access to a non trivially-copyable/POD C++ class.

   Unlike other ring buffers, an extra variable is used to track pending writes.
   This is so we can write a few bytes at a time and later mark the whole operation as complete,
   thus avoiding the issue of reading data too early from the other side.
   For example, write the size of some data first, and then the actual data.
   The reading side will only see data available once size + data is completely written and "committed".
 */
struct HeapBuffer {
   /**
      Size of the buffer, allocated in @a buf.
      If the size is fixed (stack buffer), this variable can be static.
    */
    uint32_t size;

   /**
      Current writing position, headmost position of the buffer.
      Increments when writing.
    */
    uint32_t head;

   /**
      Current reading position, last used position of the buffer.
      Increments when reading.
      head == tail means empty buffer.
    */
    uint32_t tail;

   /**
      Temporary position of head until a commitWrite() is called.
      If buffer writing fails, wrtn will be back to head position thus ignoring the last operation(s).
      If buffer writing succeeds, head will be set to this variable.
    */
    uint32_t wrtn;

   /**
      Boolean used to check if a write operation failed.
      This ensures we don't get incomplete writes.
    */
    bool invalidateCommit;

   /**
      Pointer to buffer data.
      This can be either stack or heap data, depending on the usecase.
    */
    uint8_t* buf;
};

/**
   RingBufferControl compatible struct with a relatively small stack size (4k bytes).
   @see HeapBuffer
*/
struct SmallStackBuffer {
    static const uint32_t size = 4096;
    uint32_t head, tail, wrtn;
    bool     invalidateCommit;
    uint8_t  buf[size];
};

/**
   RingBufferControl compatible struct with a relatively big stack size (16k bytes).
   @see HeapBuffer
*/
struct BigStackBuffer {
    static const uint32_t size = 16384;
    uint32_t head, tail, wrtn;
    bool     invalidateCommit;
    uint8_t  buf[size];
};

/**
   RingBufferControl compatible struct with a huge stack size (64k bytes).
   @see HeapBuffer
*/
struct HugeStackBuffer {
    static const uint32_t size = 65536;
    uint32_t head, tail, wrtn;
    bool     invalidateCommit;
    uint8_t  buf[size];
};

#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# define HeapBuffer_INIT  {0, 0, 0, 0, false, nullptr}
# define StackBuffer_INIT {0, 0, 0, false, {0}}
#else
# define HeapBuffer_INIT
# define StackBuffer_INIT
#endif

// -----------------------------------------------------------------------
// RingBufferControl templated class

/**
   DPF built-in RingBuffer class.
   RingBufferControl takes one buffer struct to take control over, and operates over it.

   This is meant for single-writer, single-reader type of control.
   Writing and reading is wait and lock-free.

   Typically usage involves:
   ```
   // definition
   HeapRingBuffer myHeapBuffer; // or RingBufferControl<HeapBuffer> class for more control

   // construction, only needed for heap buffers
   myHeapBuffer.createBuffer(8192);

   // writing data
   myHeapBuffer.writeUInt(size);
   myHeapBuffer.writeCustomData(someOtherData, size);
   myHeapBuffer.commitWrite();

   // reading data
   if (myHeapBuffer.isDataAvailableForReading())
   {
      uint32_t size;
      if (myHeapBuffer.readUInt(size) && readCustomData(&anotherData, size))
      {
          // do something with "anotherData"
      }
   }
   ```

   @see HeapBuffer
 */
template <class BufferStruct>
class RingBufferControl
{
public:
    /*
     * Constructor for uninitialised ring buffer.
     * A call to setRingBuffer is required to tied this control to a ring buffer struct;
     *
     */
    RingBufferControl() noexcept
        : buffer(nullptr),
          errorReading(false),
          errorWriting(false) {}

    /*
     * Destructor.
     */
    virtual ~RingBufferControl() noexcept {}

    // -------------------------------------------------------------------
    // check operations

    /*
     * Check if there is any data available for reading, regardless of size.
     */
    bool isDataAvailableForReading() const noexcept;

    /*
     * Check if ring buffer is empty (that is, there is nothing to read).
     */
    bool isEmpty() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(buffer != nullptr, false);

        return (buffer->buf == nullptr || buffer->head == buffer->tail);
    }

    /*
     * Get the full ringbuffer size.
     */
    uint32_t getSize() const noexcept
    {
        return buffer != nullptr ? buffer->size : 0;
    }

    /*
     * Get the size of the data available to read.
     */
    uint32_t getReadableDataSize() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(buffer != nullptr, 0);

        const uint32_t wrap = buffer->head >= buffer->tail ? 0 : buffer->size;

        return wrap + buffer->head - buffer->tail;
    }

    /*
     * Get the size of the data available to write.
     */
    uint32_t getWritableDataSize() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(buffer != nullptr, 0);

        const uint32_t wrap = buffer->tail > buffer->wrtn ? 0 : buffer->size;

        return wrap + buffer->tail - buffer->wrtn - 1;
    }

    // -------------------------------------------------------------------
    // clear/reset operations

    /*
     * Clear the entire ring buffer data, marking the buffer as empty.
     * Requires a buffer struct tied to this class.
     */
    void clearData() noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(buffer != nullptr,);

        buffer->head = 0;
        buffer->tail = 0;
        buffer->wrtn = 0;
        buffer->invalidateCommit = false;

        std::memset(buffer->buf, 0, buffer->size);
    }

    /*
     * Reset the ring buffer read and write positions, marking the buffer as empty.
     * Requires a buffer struct tied to this class.
     */
    void flush() noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(buffer != nullptr,);

        buffer->head = buffer->tail = buffer->wrtn = 0;
        buffer->invalidateCommit = false;

        errorWriting = false;
    }

    // -------------------------------------------------------------------
    // read operations

    /*
     * Read a single boolean value.
     * Returns false if reading fails.
     */
    bool readBool() noexcept
    {
        bool b = false;
        return tryRead(&b, sizeof(bool)) ? b : false;
    }

    /*
     * Read a single 8-bit byte.
     * Returns 0 if reading fails.
     */
    uint8_t readByte() noexcept
    {
        uint8_t B = 0;
        return tryRead(&B, sizeof(uint8_t)) ? B : 0;
    }

    /*
     * Read a short 16-bit integer.
     * Returns 0 if reading fails.
     */
    int16_t readShort() noexcept
    {
        int16_t s = 0;
        return tryRead(&s, sizeof(int16_t)) ? s : 0;
    }

    /*
     * Read a short unsigned 16-bit integer.
     * Returns 0 if reading fails.
     */
    uint16_t readUShort() noexcept
    {
        uint16_t us = 0;
        return tryRead(&us, sizeof(uint16_t)) ? us : 0;
    }

    /*
     * Read a regular 32-bit integer.
     * Returns 0 if reading fails.
     */
    int32_t readInt() noexcept
    {
        int32_t i = 0;
        return tryRead(&i, sizeof(int32_t)) ? i : 0;
    }

    /*
     * Read an unsigned 32-bit integer.
     * Returns 0 if reading fails.
     */
    uint32_t readUInt() noexcept
    {
        uint32_t ui = 0;
        return tryRead(&ui, sizeof(int32_t)) ? ui : 0;
    }

    /*
     * Read a long 64-bit integer.
     * Returns 0 if reading fails.
     */
    int64_t readLong() noexcept
    {
        int64_t l = 0;
        return tryRead(&l, sizeof(int64_t)) ? l : 0;
    }

    /*
     * Read a long unsigned 64-bit integer.
     * Returns 0 if reading fails.
     */
    uint64_t readULong() noexcept
    {
        uint64_t ul = 0;
        return tryRead(&ul, sizeof(int64_t)) ? ul : 0;
    }

    /*
     * Read a single-precision floating point number.
     * Returns 0 if reading fails.
     */
    float readFloat() noexcept
    {
        float f = 0.0f;
        return tryRead(&f, sizeof(float)) ? f : 0.0f;
    }

    /*
     * Read a double-precision floating point number.
     * Returns 0 if reading fails.
     */
    double readDouble() noexcept
    {
        double d = 0.0;
        return tryRead(&d, sizeof(double)) ? d : 0.0;
    }

    /*!
     * Read an arbitrary amount of data, specified by @a size.
     * data pointer must be non-null, and size > 0.
     *
     * Returns true if reading succeeds.
     * In case of failure, @a data pointer is automatically cleared by @a size bytes.
     */
    bool readCustomData(void* const data, const uint32_t size) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(data != nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(size > 0, false);

        if (tryRead(data, size))
            return true;

        std::memset(data, 0, size);
        return false;
    }

    /*!
     * Read a custom data type specified by the template typename used,
     * with size being automatically deduced by the compiler (through the use of sizeof).
     *
     * Returns true if reading succeeds.
     * In case of failure, @a type value is automatically cleared by its deduced size.
     */
    template <typename T>
    bool readCustomType(T& type) noexcept
    {
        if (tryRead(&type, sizeof(T)))
            return true;

        std::memset(&type, 0, sizeof(T));
        return false;
    }

    // -------------------------------------------------------------------
    // peek operations (returns a value without advancing read position)

    /*
     * Peek for an unsigned 32-bit integer.
     * Returns 0 if reading fails.
     */
    uint32_t peekUInt() const noexcept
    {
        uint32_t ui = 0;
        return tryPeek(&ui, sizeof(int32_t)) ? ui : 0;
    }

    /*!
     * Peek for a custom data type specified by the template typename used,
     * with size being automatically deduced by the compiler (through the use of sizeof).
     *
     * Returns true if peeking succeeds.
     * In case of failure, @a type value is automatically cleared by its deduced size.
     */
    template <typename T>
    bool peekCustomType(T& type) const noexcept
    {
        if (tryPeek(&type, sizeof(T)))
            return true;

        std::memset(&type, 0, sizeof(T));
        return false;
    }

    // -------------------------------------------------------------------
    // write operations

    /*
     * Write a single boolean value.
     */
    bool writeBool(const bool value) noexcept
    {
        return tryWrite(&value, sizeof(bool));
    }

    /*
     * Write a single 8-bit byte.
     */
    bool writeByte(const uint8_t value) noexcept
    {
        return tryWrite(&value, sizeof(uint8_t));
    }

    /*
     * Write a short 16-bit integer.
     */
    bool writeShort(const int16_t value) noexcept
    {
        return tryWrite(&value, sizeof(int16_t));
    }

    /*
     * Write a short unsigned 16-bit integer.
     */
    bool writeUShort(const uint16_t value) noexcept
    {
        return tryWrite(&value, sizeof(uint16_t));
    }

    /*
     * Write a regular 32-bit integer.
     */
    bool writeInt(const int32_t value) noexcept
    {
        return tryWrite(&value, sizeof(int32_t));
    }

    /*
     * Write an unsigned 32-bit integer.
     */
    bool writeUInt(const uint32_t value) noexcept
    {
        return tryWrite(&value, sizeof(uint32_t));
    }

    /*
     * Write a long 64-bit integer.
     */
    bool writeLong(const int64_t value) noexcept
    {
        return tryWrite(&value, sizeof(int64_t));
    }

    /*
     * Write a long unsigned 64-bit integer.
     */
    bool writeULong(const uint64_t value) noexcept
    {
        return tryWrite(&value, sizeof(uint64_t));
    }

    /*
     * Write a single-precision floating point number.
     */
    bool writeFloat(const float value) noexcept
    {
        return tryWrite(&value, sizeof(float));
    }

    /*
     * Write a double-precision floating point number.
     */
    bool writeDouble(const double value) noexcept
    {
        return tryWrite(&value, sizeof(double));
    }

    /*!
     * Write an arbitrary amount of data, specified by @a size.
     * data pointer must be non-null, and size > 0.
     */
    bool writeCustomData(const void* const data, const uint32_t size) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(data != nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(size > 0, false);

        return tryWrite(data, size);
    }

    /*!
     * Write a custom data type specified by the template typename used,
     * with size being automatically deduced by the compiler (through the use of sizeof).
     */
    template <typename T>
    bool writeCustomType(const T& type) noexcept
    {
        return tryWrite(&type, sizeof(T));
    }

    // -------------------------------------------------------------------

    /*!
     * Commit all previous write operations to the ringbuffer.
     * If a write operation has previously failed, this will reset/invalidate the previous write attempts.
     */
    bool commitWrite() noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(buffer != nullptr, false);

        if (buffer->invalidateCommit)
        {
            buffer->wrtn = buffer->head;
            buffer->invalidateCommit = false;
            return false;
        }

        // nothing to commit?
        DISTRHO_SAFE_ASSERT_RETURN(buffer->head != buffer->wrtn, false);

        // all ok
        buffer->head = buffer->wrtn;
        errorWriting = false;
        return true;
    }

    // -------------------------------------------------------------------

    /*
     * Tie this ring buffer control to a ring buffer struct, optionally clearing its data.
     */
    void setRingBuffer(BufferStruct* const ringBuf, const bool clearRingBufferData) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(buffer != ringBuf,);

        buffer = ringBuf;

        if (clearRingBufferData && ringBuf != nullptr)
            clearData();
    }

    // -------------------------------------------------------------------

protected:
    /** @internal try reading from the buffer, can fail. */
    bool tryRead(void* const buf, const uint32_t size) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(buffer != nullptr, false);
       #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wtautological-pointer-compare"
       #endif
        DISTRHO_SAFE_ASSERT_RETURN(buffer->buf != nullptr, false);
       #if defined(__clang__)
        #pragma clang diagnostic pop
       #endif
        DISTRHO_SAFE_ASSERT_RETURN(buf != nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(size > 0, false);
        DISTRHO_SAFE_ASSERT_RETURN(size < buffer->size, false);

        // empty
        if (buffer->head == buffer->tail)
            return false;

        uint8_t* const bytebuf = static_cast<uint8_t*>(buf);

        const uint32_t head = buffer->head;
        const uint32_t tail = buffer->tail;
        const uint32_t wrap = head > tail ? 0 : buffer->size;

        if (size > wrap + head - tail)
        {
            if (! errorReading)
            {
                errorReading = true;
                d_stderr2("RingBuffer::tryRead(%p, %lu): failed, not enough space", buf, (ulong)size);
            }
            return false;
        }

        uint32_t readto = tail + size;

        if (readto > buffer->size)
        {
            readto -= buffer->size;

            if (size == 1)
            {
                std::memcpy(bytebuf, buffer->buf + tail, 1);
            }
            else
            {
                const uint32_t firstpart = buffer->size - tail;
                std::memcpy(bytebuf, buffer->buf + tail, firstpart);
                std::memcpy(bytebuf + firstpart, buffer->buf, readto);
            }
        }
        else
        {
            std::memcpy(bytebuf, buffer->buf + tail, size);

            if (readto == buffer->size)
                readto = 0;
        }

        buffer->tail = readto;
        errorReading = false;
        return true;
    }

    /** @internal try reading from the buffer, can fail. */
    bool tryPeek(void* const buf, const uint32_t size) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(buffer != nullptr, false);
       #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wtautological-pointer-compare"
       #endif
        DISTRHO_SAFE_ASSERT_RETURN(buffer->buf != nullptr, false);
       #if defined(__clang__)
        #pragma clang diagnostic pop
       #endif
        DISTRHO_SAFE_ASSERT_RETURN(buf != nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(size > 0, false);
        DISTRHO_SAFE_ASSERT_RETURN(size < buffer->size, false);

        // empty
        if (buffer->head == buffer->tail)
            return false;

        uint8_t* const bytebuf = static_cast<uint8_t*>(buf);

        const uint32_t head = buffer->head;
        const uint32_t tail = buffer->tail;
        const uint32_t wrap = head > tail ? 0 : buffer->size;

        if (size > wrap + head - tail)
            return false;

        uint32_t readto = tail + size;

        if (readto > buffer->size)
        {
            readto -= buffer->size;

            if (size == 1)
            {
                std::memcpy(bytebuf, buffer->buf + tail, 1);
            }
            else
            {
                const uint32_t firstpart = buffer->size - tail;
                std::memcpy(bytebuf, buffer->buf + tail, firstpart);
                std::memcpy(bytebuf + firstpart, buffer->buf, readto);
            }
        }
        else
        {
            std::memcpy(bytebuf, buffer->buf + tail, size);

            if (readto == buffer->size)
                readto = 0;
        }

        return true;
    }

    /** @internal try writing to the buffer, can fail. */
    bool tryWrite(const void* const buf, const uint32_t size) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(buffer != nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(buf != nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(size > 0, false);
        DISTRHO_SAFE_ASSERT_UINT2_RETURN(size < buffer->size, size, buffer->size, false);

        const uint8_t* const bytebuf = static_cast<const uint8_t*>(buf);

        const uint32_t tail = buffer->tail;
        const uint32_t wrtn = buffer->wrtn;
        const uint32_t wrap = tail > wrtn ? 0 : buffer->size;

        if (size >= wrap + tail - wrtn)
        {
            if (! errorWriting)
            {
                errorWriting = true;
                d_stderr2("RingBuffer::tryWrite(%p, %lu): failed, not enough space", buf, (ulong)size);
            }
            buffer->invalidateCommit = true;
            return false;
        }

        uint32_t writeto = wrtn + size;

        if (writeto > buffer->size)
        {
            writeto -= buffer->size;

            if (size == 1)
            {
                std::memcpy(buffer->buf, bytebuf, 1);
            }
            else
            {
                const uint32_t firstpart = buffer->size - wrtn;
                std::memcpy(buffer->buf + wrtn, bytebuf, firstpart);
                std::memcpy(buffer->buf, bytebuf + firstpart, writeto);
            }
        }
        else
        {
            std::memcpy(buffer->buf + wrtn, bytebuf, size);

            if (writeto == buffer->size)
                writeto = 0;
        }

        buffer->wrtn = writeto;
        return true;
    }

private:
    /** Buffer struct pointer. */
    BufferStruct* buffer;

    /** Whether read errors have been printed to terminal. */
    bool errorReading;

    /** Whether write errors have been printed to terminal. */
    bool errorWriting;

    DISTRHO_PREVENT_VIRTUAL_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPYABLE(RingBufferControl)
};

template <class BufferStruct>
inline bool RingBufferControl<BufferStruct>::isDataAvailableForReading() const noexcept
{
    return (buffer != nullptr && buffer->head != buffer->tail);
}

template <>
inline bool RingBufferControl<HeapBuffer>::isDataAvailableForReading() const noexcept
{
    return (buffer != nullptr && buffer->buf != nullptr && buffer->head != buffer->tail);
}

// -----------------------------------------------------------------------
// RingBuffer using heap space

/**
   RingBufferControl with a heap buffer.
   This is a convenience class that provides a method for creating and destroying the heap data.
   Requires the use of createBuffer(uint32_t) to make the ring buffer usable.
*/
class HeapRingBuffer : public RingBufferControl<HeapBuffer>
{
public:
    /** Constructor. */
    HeapRingBuffer() noexcept
        : heapBuffer(HeapBuffer_INIT)
    {
#ifndef DISTRHO_PROPER_CPP11_SUPPORT
        std::memset(&heapBuffer, 0, sizeof(heapBuffer));
#endif
    }

    /** Destructor. */
    ~HeapRingBuffer() noexcept override
    {
        if (heapBuffer.buf == nullptr)
            return;

        delete[] heapBuffer.buf;
        heapBuffer.buf = nullptr;
    }

    /** Create a buffer of the specified size. */
    bool createBuffer(const uint32_t size) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(heapBuffer.buf == nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(size > 0,  false);

        const uint32_t p2size = d_nextPowerOf2(size);

        try {
            heapBuffer.buf = new uint8_t[p2size];
        } DISTRHO_SAFE_EXCEPTION_RETURN("HeapRingBuffer::createBuffer", false);

        heapBuffer.size = p2size;
        setRingBuffer(&heapBuffer, true);
        return true;
    }

    /** Delete the previously allocated buffer. */
    void deleteBuffer() noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(heapBuffer.buf != nullptr,);

        setRingBuffer(nullptr, false);

        delete[] heapBuffer.buf;
        heapBuffer.buf  = nullptr;
        heapBuffer.size = 0;
    }

    void copyFromAndClearOther(HeapRingBuffer& other)
    {
        DISTRHO_SAFE_ASSERT_RETURN(other.heapBuffer.size == heapBuffer.size,);

        std::memcpy(&heapBuffer, &other.heapBuffer, sizeof(HeapBuffer) - sizeof(uint8_t*));
        std::memcpy(heapBuffer.buf, other.heapBuffer.buf, sizeof(uint8_t) * heapBuffer.size);
        other.clearData();
    }

private:
    /** The heap buffer used for this class. */
    HeapBuffer heapBuffer;

    DISTRHO_PREVENT_VIRTUAL_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPYABLE(HeapRingBuffer)
};

// -----------------------------------------------------------------------
// RingBuffer using small stack space

/**
   RingBufferControl with an included small stack buffer.
   No setup is necessary, this class is usable as-is.
*/
class SmallStackRingBuffer : public RingBufferControl<SmallStackBuffer>
{
public:
    /** Constructor. */
    SmallStackRingBuffer() noexcept
        : stackBuffer(StackBuffer_INIT)
    {
#ifndef DISTRHO_PROPER_CPP11_SUPPORT
        std::memset(&stackBuffer, 0, sizeof(stackBuffer));
#endif
        setRingBuffer(&stackBuffer, true);
    }

private:
    /** The small stack buffer used for this class. */
    SmallStackBuffer stackBuffer;

    DISTRHO_PREVENT_VIRTUAL_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPYABLE(SmallStackRingBuffer)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_RING_BUFFER_HPP_INCLUDED
