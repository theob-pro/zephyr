#include <stdint.h>

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
int h8(const uint8_t k[16], const uint8_t s[16], const uint8_t key_id[4], uint8_t res[16]);