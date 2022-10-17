#include <zephyr/bluetooth/crypto_toolbox/h8.h>

int bt_crypto_toolbox_h8(const uint8_t k[16], const uint8_t s[16], const uint8_t key_id[4], uint8_t res[16])
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
