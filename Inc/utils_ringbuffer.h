/**
 * \file
 *
 * \brief Ringbuffer declaration.
 *
 * Copyright (c) 2014-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
#ifndef _UTILS_RINGBUFFER_H_INCLUDED
#define _UTILS_RINGBUFFER_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif
#define ERR_NONE 0
#define ERR_INVALID_DATA -1
#define ERR_NO_CHANGE -2
#define ERR_ABORTED -3
#define ERR_BUSY -4
#define ERR_SUSPEND -5
#define ERR_IO -6
#define ERR_REQ_FLUSHED -7
#define ERR_TIMEOUT -8
#define ERR_BAD_DATA -9
#define ERR_NOT_FOUND -10
#define ERR_UNSUPPORTED_DEV -11
#define ERR_NO_MEMORY -12
#define ERR_INVALID_ARG -13
#define ERR_BAD_ADDRESS -14
#define ERR_BAD_FORMAT -15
#define ERR_BAD_FRQ -16
#define ERR_DENIED -17
#define ERR_ALREADY_INITIALIZED -18
#define ERR_OVERFLOW -19
#define ERR_NOT_INITIALIZED -20
#define ERR_SAMPLERATE_UNAVAILABLE -21
#define ERR_RESOLUTION_UNAVAILABLE -22
#define ERR_BAUDRATE_UNAVAILABLE -23
#define ERR_PACKET_COLLISION -24
#define ERR_PROTOCOL -25
#define ERR_PIN_MUX_INVALID -26
#define ERR_UNSUPPORTED_OP -27
#define ERR_NO_RESOURCE -28
#define ERR_NOT_READY -29
#define ERR_FAILURE -30
#define ERR_WRONG_LENGTH -31

	/**
	 * \addtogroup doc_driver_hal_utils_ringbuffer
	 *
	 * @{
	 */

#include "config.h"

	/**
	 * \brief Ring buffer element type
	 */
	struct ringbuffer
	{
		uint8_t *buf;		  /** Buffer base address */
		uint16_t size;		  /** Buffer size */
		uint16_t read_index;  /** Buffer read index */
		uint16_t write_index; /** Buffer write index */
	};

	/**
	 * \brief Ring buffer init
	 *
	 * \param[in] rb The pointer to a ring buffer structure instance
	 * \param[in] buf Space to store the data
	 * \param[in] size The buffer length, must be aligned with power of 2
	 *
	 * \return ERR_NONE on success, or an error code on failure.
	 */
	int16_t ringbuffer_init(struct ringbuffer *const rb, void *buf, uint16_t size);

	/**
	 * \brief Get one byte from ring buffer, the user needs to handle the concurrent
	 * access on buffer via put/get/flush
	 *
	 * \param[in] rb The pointer to a ring buffer structure instance
	 * \param[in] data One byte space to store the read data
	 *
	 * \return ERR_NONE on success, or an error code on failure.
	 */
	int16_t ringbuffer_get(struct ringbuffer *const rb, uint8_t *dat);

	/**
	 * \brief Get multiple byte from ringbuffer
	 *
	 */
	uint16_t ringbuffer_gets(struct ringbuffer *const rb, uint8_t *buf, uint16_t size);

	/**
	 * \brief Put one byte to ring buffer, the user needs to handle the concurrent access
	 * on buffer via put/get/flush
	 *
	 * \param[in] rb The pointer to a ring buffer structure instance
	 * \param[in] data One byte data to be put into ring buffer
	 *
	 * \return ERR_NONE on success, or an error code on failure.
	 */
	int16_t ringbuffer_put(struct ringbuffer *const rb, uint8_t dat);

	/**
	 * \brief Return the element number of ring buffer
	 *
	 * \param[in] rb The pointer to a ring buffer structure instance
	 *
	 * \return The number of elements in ring buffer [0, rb->size]
	 */
	uint16_t ringbuffer_num(const struct ringbuffer *const rb);

	/**
	 * \brief Flush ring buffer, the user needs to handle the concurrent access on buffer
	 * via put/get/flush
	 *
	 * \param[in] rb The pointer to a ring buffer structure instance
	 *
	 * \return ERR_NONE on success, or an error code on failure.
	 */
	uint16_t ringbuffer_flush(struct ringbuffer *const rb);

	/**@}*/

#ifdef __cplusplus
}
#endif
#endif /* _UTILS_RINGBUFFER_H_INCLUDED */
