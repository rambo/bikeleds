import crc8
import msgpack
from cobs import cobs
import serial

def pack(datain, pkgidx=1):
    """Pack into msgpacketizer compatible form"""
    check = crc8.crc8()
    packed = msgpack.packb(datain)
    check.update(packed)
    return bytes([pkgidx]) + packed + check.digest()

def unpack(bytesin):
    """unpack from msgpacketizer form"""
    if bytesin[-1] == 0:
        bytesin = bytesin[:-1]
    cobs.decode(bytesin)
    idx = bytesin[1]
    pktcrc = bytesin[-1]
    packed = bytesin[1:-1]
    check = crc8.crc8()
    check.update(packed)
    expectedcrc = check.digest()
    if expectedcrc != pktcrc:
        raise ValueError("packet checksum {} does not match expected {}".format(pktcrc, expectedcrc))
    data = msgpack.unpackb()
    return (idx, data)

PORT = serial.Serial("/dev/tty.DucatiLEDs-ESP32SPP", timeout=1.0)
#PORT = serial.Serial("/dev/tty.SLAB_USBtoUART", 115200, timeout=1.0)

def send(datain, pkgidx=1):
    encoded = cobs.encode(pack(datain, pkgidx))
    PORT.write(encoded+ b"\0")
    PORT.flush()
    print("sent: {}".format(encoded))
