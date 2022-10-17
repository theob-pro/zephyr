/* Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/ztest.h>

#include <errno.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/cmac_mode.h>

#include <zephyr/bluetooth/crypto_toolbox/h8.h>

ZTEST_SUITE(crypto_toolbox_h8, NULL, NULL, NULL, NULL, NULL);

static bool is_equal(const uint8_t gsk_a[16], const uint8_t gsk_b[16])
{
	for (unsigned int i = 0; i < 16; i++) {
		if (gsk_a[i] != gsk_b[i]) {
			return false;
		}
	}

	return true;
}

/* Cypher based Message Authentication Code (CMAC) with AES 128 bit
 *
 * Input    : key    ( 128-bit key )
 *          : in     ( message to be authenticated )
 *          : len    ( length of the message in octets )
 * Output   : out    ( message authentication code )
 */
static int aes_cmac(const uint8_t *key, const uint8_t *in, size_t len,
			   uint8_t *out)
{
	struct tc_aes_key_sched_struct sched;
	struct tc_cmac_struct state;

	if (tc_cmac_setup(&state, key, &sched) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	if (tc_cmac_update(&state, in, len) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	if (tc_cmac_final(out, &state) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	return 0;
}

/**
 * @brief Cryptograhic Toolbox function H8
 *
 * Defined in Core Vol 6, part E 1.1.1.
 *
 * @note This function is purely a shorthand for the calculation. The paramters
 * are therefore intentionally not assigned meaning.
 *
 * Pseudocode: `aes_cmac(key=aes_cmac(key=s, plaintext=k), plaintext=key_id)`
 *
 * @param k (128-bit number in big endian)
 * @param s (128-bit number in big endian)
 * @param key_id (32-bit number in big endian)
 * @param[out] res (128-bit number in big endian)
 * @retval 0 Computation was successful. @p res contains the result.
 * @retval @-EIO
 */
int crypto_toolbox_h8(const uint8_t k[16], const uint8_t s[16], const uint8_t key_id[4], uint8_t res[16])
{
	int err;
	uint8_t ik[16];

	err = aes_cmac(s, k, 16, ik);
	if (err) {
		return err;
	}

	err = aes_cmac(ik, key_id, 4, res);
	if (err) {
		return err;
	}

	return 0;
}

static void result_gsk_is(const uint8_t k[16], const uint8_t s[16], const uint8_t key_id[4], const uint8_t expected_gsk[16])
{
	uint8_t gsk[16];

	crypto_toolbox_h8(k, s, key_id, gsk);
	zassert_true(is_equal(gsk, expected_gsk), "Result doesn't match expected value.");
}

ZTEST(crypto_toolbox_h8, test_result)
{
	uint8_t expected_gsk[16] = {
		0xe5, 0xe5, 0xbe, 0xba,
		0xae, 0x72, 0x28, 0xe7,
		0x22, 0xa3, 0x89, 0x04,
		0xed, 0x35, 0x0f, 0x6d
	};
	
	uint8_t k[16] = {
		0xec, 0x02, 0x34, 0xa3,
		0x57, 0xc8, 0xad, 0x05,
		0x34, 0x10, 0x10, 0xa6,
		0x0a, 0x39, 0x7d, 0x9b
	};
	uint8_t s[16] = {
		0x15, 0x36, 0xd1, 0x8d,
		0xe3, 0xd2, 0x0d, 0xf9,
		0x9b, 0x70, 0x44, 0xc1,
		0x2f, 0x9e, 0xd5, 0xba
	};
	uint8_t key_id[4] = {0xcc, 0x03, 0x01, 0x48};
	
	result_gsk_is(k, s, key_id, expected_gsk);
}
