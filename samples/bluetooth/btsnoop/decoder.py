#!/usr/bin/env python3

import serial
import argparse
from datetime import datetime
import struct

LINKTYPE_BLUETOOTH_HCI_H4 = 187

SLIP_END     = 0xC0.to_bytes(1, 'little')
SLIP_ESC     = 0xDB.to_bytes(1, 'little')
SLIP_ESC_END = 0xDC.to_bytes(1, 'little')
SLIP_ESC_ESC = 0xDD.to_bytes(1, 'little')

def get_pcap_hdr():
        magic_number = 0xA1B2C3D4
        major_version = 2
        minor_version = 4
        reserved1 = 0x00.to_bytes(32, 'big')
        reserved2 = 0x00.to_bytes(32, 'big')
        fcs_r_p_reserved3 = 0x00.to_bytes(16, 'big')
        link_type = LINKTYPE_BLUETOOTH_HCI_H4.to_bytes(16, 'big')
        struct.pack('>IHH32s32s16s',
                magic_number,
                major_version,
                minor_version,
                reserved1,
                reserved2,
                fcs_r_p_reserved3,
                link_type
        )

def btsnoop2pcap(data):
        pass

"""
BT Snoop packet format:
+------------------+--------------+
| original length  | 32-bit uint  |
| included length  | 32-bit uint  |
| pkt flags        | 32-bit       |
| cumulative drops | 32-bit uint  |
| timestamp in us  | 64-bit int   |
| pkt data         | var length   |
+------------------+--------------+
all int are stored in big-endian with the high-order bits first
"""
class BtsnoopPkt2PcapPkt():
        btsnoop_pkt: bytes
        btsnoop_pkt_min_len = 24
        btsnoop_pkt_data_len = 0

        timestamp_s: bytes
        timestamp_us: bytes
        captured_pkt_len: bytes
        original_pkt_len: bytes
        data: bytes

        def btsnoop_get_original_len(self) -> bytes:
                original_len = self.btsnoop_pkt[:4]
                return self.btsnoop_pkt[:4]

        def btsnoop_get_captured_len(self) -> bytes:
                return self.btsnoop_pkt[4:8]

        def btsnoop_get_drops(self) -> bytes:
                return self.btsnoop_pkt[8:12]

        def btsnoop_get_ts(self) -> bytes:
                return self.btsnoop_pkt[12:24]

        def btsnoop_get_data(self) -> bytes:
                return self.btsnoop_pkt[24:]

        def add_btsnoop_byte(self, byte: bytes):
                """
                        Add a btsnoop byte to convert.
                        Return False if the packet isn't complete
                        Return True if the packet is complete
                """
                btsnoop_pkt_len = len(self.btsnoop_pkt)
                btsnoop_pkt_captured_len = int.from_bytes(self.btsnoop_get_captured_len(), 'big')
                if btsnoop_pkt_len < self.btsnoop_pkt_min_len or btsnoop_pkt_len - self.btsnoop_pkt_min_len < btsnoop_pkt_captured_len:
                        self.btsnoop_pkt = self.btsnoop_pkt + byte
                        return False

                return True

        def get_pcap_pkt(self) -> bytes:
                self.timestamp_s = self.btsnoop_get_ts() // 1_000_000
                self.timestamp_us = self.btsnoop_get_ts() % 1_000_000
                self.captured_pkt_len = self.btsnoop_get_captured_len()
                self.original_pkt_len = self.btsnoop_get_original_len()
                self.data = self.btsnoop_get_data()

def new_file(filename):
        fname = filename
        if fname == None:
                current_datetime = datetime.now()
                datetime_format = "%Y%m%d-%H%M%S"
                fname = f"{current_datetime.strftime(datetime_format)}.btsnoop"
        print(f"Creating new file: {fname}")
        return open(fname, 'wb')

def main():
        parser = argparse.ArgumentParser(description="Connect to UART, decode SLIP and write the data in a BT Snoop file.")
        parser.add_argument('-t', '--tty', required=True, help='TTY device path')
        parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate')
        parser.add_argument('-o', '--output', default=None, help="Output file for BT Snoop data")

        args = parser.parse_args()

        ser = serial.Serial(args.tty, args.baudrate)
        f = None
        esc = False

        print("Waiting SLIP_END")

        try:
                while True:
                        data = ser.read() # block until it get 1 byte
                        decoded_data = None

                        if data == SLIP_END and not esc:
                                print("Encountered SLIP_END")
                                if f != None:
                                        f.close()
                                f = new_file(args.output)
                                continue
                        elif data == SLIP_ESC:
                                esc = True
                                continue
                        elif data == SLIP_ESC_END and esc:
                                decoded_data = SLIP_END
                                esc = False
                        elif data == SLIP_ESC_ESC and esc:
                                decoded_data = SLIP_ESC
                                esc = False
                        else:
                                decoded_data = data

                        if f != None:
                                f.write(decoded_data)
        except KeyboardInterrupt:
                if f != None:
                        print("\nClosing file...")
                        f.close()

if __name__ == "__main__":
        main()
