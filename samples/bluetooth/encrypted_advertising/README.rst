.. _bluetooth_encrypted_advertising_sample:

Bluetooth: Encrypted Advertising
################################

Overview
********

This sample demonstrates the usage of the encrypted advertising feature. This
include:

 - the exchange of the session key and the initialization vector using the Key
   Material characteristic;
 - the encryption of advertising payloads;
 - the decryption of those advertising payloads;
 - and finally the update of the Randomizer field whenever the RPA is changed.

To use the `bt_ead_encrypt` and `bt_ead_decrypt` functions you need to enable
the Kconfig symbol :kconfig:option:`CONFIG_BT_EAD`.

This sample uses extended advertising, this is **not** mandatory to use the
feature. The encrypted adverting feature can be used with legacy advertising as
well.

Hardware Setup
**************

This sample use two applications, two devices need to be setup.
The first one should be flashed with the central and the second one with the
peripheral.

The two devices should automatically connect based on the device name advertised
by the peripheral.

Building and Running
********************

This sample can be found under
:zephyr_file:`samples/bluetooth/encrypted_advertising` in the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.

Here are the outputs you should get by default:

Peripheral:

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v3.3.0-1872-g6fac3c7581dc ***
        <inf> ead_peripheral_sample: Advertising data size: 64
        <inf> ead_peripheral_sample: Advertising data size: 64


Central:

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v3.3.0-1872-g6fac3c7581dc ***
        <inf> ead_central_sample: Received data size: 64
        <inf> ead_central_sample: len : 10
        <inf> ead_central_sample: type: 0x09
        <inf> ead_central_sample: data:
                                  45 41 44 20 53 61 6d 70  6c 65                   |EAD Samp le
        <inf> ead_central_sample: len : 8
        <inf> ead_central_sample: type: 0xff
        <inf> ead_central_sample: data:
                                  05 f1 5a 65 70 68 79 72                          |..Zephyr
        <inf> ead_central_sample: len : 7
        <inf> ead_central_sample: type: 0xff
        <inf> ead_central_sample: data:
                                  05 f1 49 d2 f4 55 76                             |..I..Uv
        <inf> ead_central_sample: len : 4
        <inf> ead_central_sample: type: 0xff
        <inf> ead_central_sample: data:
                                  05 f1 c1 25                                      |...%
        <inf> ead_central_sample: len : 3
        <inf> ead_central_sample: type: 0xff
        <inf> ead_central_sample: data:
                                  05 f1 17                                         |...
