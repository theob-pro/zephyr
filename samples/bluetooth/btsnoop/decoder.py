#!/usr/bin/env python3

import serial
import argparse
from datetime import datetime

SLIP_END     = 0xC0.to_bytes(1, 'little')
SLIP_ESC     = 0xDB.to_bytes(1, 'little')
SLIP_ESC_END = 0xDC.to_bytes(1, 'little')
SLIP_ESC_ESC = 0xDD.to_bytes(1, 'little')

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
