/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ringbuffer.h"

void ringbuf_init(ringbuf_t *fifo, uint8_t *buf, uint32_t size)
{
    if (!fifo || !buf || !size)
        return;

    memset(fifo, 0, sizeof(ringbuf_t));
    fifo->size = size;
    fifo->buffer = buf;
}

/**
  * \brief  Removes the entire FIFO contents.
  * \param  [in] fifo: The fifo to be emptied.
  * \return None.
  */
void ringbuf_reset(ringbuf_t *fifo)
{
    fifo->write = fifo->read = 0;
    fifo->data_len = 0;
}

/**
  * \brief  Returns the size of the FIFO in bytes.
  * \param  [in] fifo: The fifo to be used.
  * \return The size of the FIFO.
  */
static inline uint32_t ringbuf_size(ringbuf_t *fifo)
{
    return fifo->size;
}

/**
  * \brief  Returns the number of used bytes in the FIFO.
  * \param  [in] fifo: The fifo to be used.
  * \return The number of used bytes.
  */
uint32_t ringbuf_len(ringbuf_t *fifo)
{
    return fifo->data_len;
}

/**
  * \brief  Returns the number of bytes available in the FIFO.
  * \param  [in] fifo: The fifo to be used.
  * \return The number of bytes available.
  */
uint32_t ringbuf_avail(ringbuf_t *fifo)
{
    return ringbuf_size(fifo) - ringbuf_len(fifo);
}

/**
  * \brief  Is the FIFO empty?
  * \param  [in] fifo: The fifo to be used.
  * \retval true:      Yes.
  * \retval false:     No.
  */
bool ringbuf_is_empty(ringbuf_t *fifo)
{
    return ringbuf_len(fifo) == 0;
}

/**
  * \brief  Is the FIFO full?
  * \param  [in] fifo: The fifo to be used.
  * \retval true:      Yes.
  * \retval false:     No.
  */
bool ringbuf_is_full(ringbuf_t *fifo)
{
    return ringbuf_avail(fifo) == 0;
}

/**
  * \brief  Puts some data into the FIFO.
  * \param  [in] fifo: The fifo to be used.
  * \param  [in] in:   The data to be added.
  * \param  [in] len:  The length of the data to be added.
  * \return The number of bytes copied.
  * \note   This function copies at most @len bytes from the @in into
  *         the FIFO depending on the free space, and returns the number
  *         of bytes copied.
  */
uint32_t ringbuf_in(ringbuf_t *fifo, const void *datptr, uint32_t len)
{
    uint32_t writelen = 0, tmplen = 0;

    if(ringbuf_is_full(fifo))
        return 0;

    tmplen = fifo->size - fifo->data_len;
    writelen = tmplen > len ? len : tmplen;

    if(fifo->write < fifo->read) {
        memcpy((void*)&fifo->buffer[fifo->write], (void*)datptr, writelen);
    } else {
        tmplen = fifo->size - fifo->write;
        if(writelen <= tmplen) {
            memcpy((void*)&fifo->buffer[fifo->write], (void*)datptr, writelen);
        } else {
            memcpy((void*)&fifo->buffer[fifo->write], (void*)datptr, tmplen);
            memcpy((void*)fifo->buffer, (uint8_t*)datptr + tmplen, writelen - tmplen);
        }
    }

    fifo->write = (fifo->write + writelen) % fifo->size;
    fifo->data_len += writelen;

    return writelen;
}

/**
  * \brief  Gets some data from the FIFO.
  * \param  [in] fifo: The fifo to be used.
  * \param  [in] out:  Where the data must be copied.
  * \param  [in] len:  The size of the destination buffer.
  * \return The number of copied bytes.
  * \note   This function copies at most @len bytes from the FIFO into
  *         the @out and returns the number of copied bytes.
  */
uint32_t ringbuf_out(ringbuf_t *fifo, void *outbuf, uint32_t len)
{
    uint32_t readlen = 0, tmplen = 0;
    if(ringbuf_is_empty(fifo))
        return 0;

    uint32_t data_len = fifo->data_len;
    readlen = len > data_len ? data_len : len;
    tmplen = fifo->size - fifo->read;

    if(NULL != outbuf) {
        if(readlen <= tmplen) {
            memcpy((void*)outbuf, (void*)&fifo->buffer[fifo->read], readlen);
        } else {
            memcpy((void*)outbuf,(void*)&fifo->buffer[fifo->read], tmplen);
            memcpy((uint8_t*)outbuf + tmplen,(void*)fifo->buffer,readlen - tmplen);
        }
    }

    fifo->read = (fifo->read + readlen) % fifo->size;
    fifo->data_len -= readlen;

    return readlen;
}

/**
  * \brief  Move FIFO buffer to another FIFO.
  * \param  [in] fifo_in: The fifo to be used.
  * \param  [in] fifo_out: The fifo to be used.
  * \return The number of copied bytes.
  * \note   This function copies at most @len bytes from the FIFO into
  *         the @out and returns the number of copied bytes.
  */
uint32_t ringbuf_move(ringbuf_t *fifo_in, ringbuf_t *fifo_out)
{
    uint32_t readlen = 0, tmplen_out = 0;
    if(ringbuf_is_empty(fifo_out))
        return 0;

    int len = ringbuf_avail(fifo_in);

    uint32_t data_len = fifo_out->data_len;
    readlen = len > data_len ? data_len : len;
    tmplen_out = fifo_out->size - fifo_out->read;

    if(readlen <= tmplen_out) {
        ringbuf_in(fifo_in, (void*)&fifo_out->buffer[fifo_out->read], readlen);
    } else {
        ringbuf_in(fifo_in, (void*)&fifo_out->buffer[fifo_out->read], tmplen_out);
        ringbuf_in(fifo_in, (void*)fifo_out->buffer, readlen - tmplen_out);
    }

    fifo_out->read = (fifo_out->read + readlen) % fifo_out->size;
    fifo_out->data_len -= readlen;

    return readlen;
}
