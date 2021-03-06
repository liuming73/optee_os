/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <kernel/tee_common.h>
#include <kernel/chip_services.h>
#include <kernel/tee_misc.h>
#include <mm/core_memprot.h>
#include <kernel/tee_common_otp.h>
#include <trace.h>

static uint8_t tee_b2hs_add_base(uint8_t in)
{
	if (in > 9)
		return in + 55;
	else
		return in + 48;
}

static int tee_hs2b_rem_base(uint8_t in, uint8_t *out)
{
	if (in < 48 || in > 70 || (in > 57 && in < 65))
		return -1;

	if (in < 58)
		*out = in - 48;
	else
		*out = in - 55;

	return 0;
}

uint32_t tee_b2hs(uint8_t *b, uint8_t *hs, uint32_t blen, uint32_t hslen)
{
	uint32_t i = 0;

	if (blen * 2 + 1 > hslen)
		return 0;

	for (; i < blen; i++) {
		hs[i * 2 + 1] = tee_b2hs_add_base(b[i] & 0xf);
		hs[i * 2] = tee_b2hs_add_base(b[i] >> 4);
	}
	hs[blen * 2] = 0;

	return blen * 2;
}

uint32_t tee_hs2b(uint8_t *hs, uint8_t *b, uint32_t hslen, uint32_t blen)
{
	uint32_t i = 0;
	uint32_t len = TEE_HS2B_BBUF_SIZE(hslen);
	uint8_t hi;
	uint8_t lo;

	if (len > blen)
		return 0;

	for (; i < len; i++) {
		if (tee_hs2b_rem_base(hs[i * 2], &hi))
			return 0;
		if (tee_hs2b_rem_base(hs[i * 2 + 1], &lo))
			return 0;
		b[i] = (hi << 4) + lo;
	}

	return len;
}

static bool is_valid_conf_and_notnull_size(
		vaddr_t b, size_t bl, vaddr_t a, size_t al)
{
	/* invalid config return false */
	if ((b + bl < b) || (a + al < a))
		return false;
	/* null sized areas are never inside / outside / overlap */
	if (!bl || !al)
		return false;
	return true;
}

/* Returns true when buffer 'b' is fully contained in area 'a' */
bool _core_is_buffer_inside(vaddr_t b, size_t bl, vaddr_t a, size_t al)
{
	/* invalid config or "null size" return false */
	if (!is_valid_conf_and_notnull_size(b, bl, a, al))
		return false;

	if ((b >= a) && (b + bl <= a + al))
		return true;
	return false;
}

/* Returns true when buffer 'b' is fully contained in area 'a' */
bool _core_is_buffer_outside(vaddr_t b, size_t bl, vaddr_t a, size_t al)
{
	/* invalid config or "null size" return false */
	if (!is_valid_conf_and_notnull_size(b, bl, a, al))
		return false;

	if ((b + bl <= a) || (b >= a + al))
		return true;
	return false;
}

/* Returns true when buffer 'b' intersects area 'a' */
bool _core_is_buffer_intersect(vaddr_t b, size_t bl, vaddr_t a, size_t al)
{
	/* invalid config or "null size" return false */
	if (!is_valid_conf_and_notnull_size(b, bl, a, al))
		return false;

	if ((b + bl <= a) || (b >= a + al))
		return false;
	return true;
}

int uuid2str(char *dst, TEE_UUID *uuid)
{
	if (dst == NULL)
		return 0;

	memset(dst, 0, TEE_UUID_STRING_LEN);
	return snprintf(dst, TEE_UUID_STRING_LEN,
			"%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
			(unsigned int)uuid->timeLow,
			(unsigned int)uuid->timeMid,
			(unsigned int)uuid->timeHiAndVersion,
			(unsigned int)uuid->clockSeqAndNode[0],
			(unsigned int)uuid->clockSeqAndNode[1],
			(unsigned int)uuid->clockSeqAndNode[2],
			(unsigned int)uuid->clockSeqAndNode[3],
			(unsigned int)uuid->clockSeqAndNode[4],
			(unsigned int)uuid->clockSeqAndNode[5],
			(unsigned int)uuid->clockSeqAndNode[6],
			(unsigned int)uuid->clockSeqAndNode[7]);
}
