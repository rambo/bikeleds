#!/usr/bin/env python3
"""Tests to send commands to Lighmodes Night Shift LED blinker"""
from typing import Any, List
import asyncio
import uuid
from bleak import BleakScanner, BleakClient

NOTIFY_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E".lower()
WRITE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E".lower()
SCAN_SERVICE_UUIDS = [WRITE_UUID]

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


async def scan_devices():
    devices = await BleakScanner.discover(service_uuids=SCAN_SERVICE_UUIDS)
    for d in devices:
        print(d)
        print(d.metadata)


async def scan_services():
    devices = await BleakScanner.discover(service_uuids=SCAN_SERVICE_UUIDS, timeout=5.0)
    for dev in devices:
        async with BleakClient(dev) as client:
            svcs = await client.get_services()
            print("Services:")
            for service in svcs:
                print(service)


async def write_helper(client: Any, char: Any, datain: List, pkgidx: int = 1) -> Any:
    encoded = cobs.encode(pack(datain, pkgidx))
    resp = await client.write_gatt_char(char, encoded+ b"\0", response=False)
    print("sent: {}".format(encoded))
    print(repr(resp))
    return resp



def handle_notify(hdl: int, data: bytearray):
    print(f"received on {hdl}: {repr(data)}")


async def write_all():
    devices = await BleakScanner.discover(service_uuids=SCAN_SERVICE_UUIDS, timeout=3.0)
    for dev in devices:
        async with BleakClient(dev) as client:
            print(f"Starting notify on {NOTIFY_UUID}")
            await client.start_notify(NOTIFY_UUID, handle_notify)
            print(f"writing to prev")
            await client.write_gatt_char(NOTIFY_UUID, b"\0")

            srvs = await client.get_services()
            srv = srvs.get_service(WRITE_UUID)
            chars = srv.characteristics
            print(f"srv: {repr(srv)}, chars: {repr(chars)}")
            for char in chars:
                print(f"char: {repr(char)} -> str: {char}")



            await write_helper(client, WRITE_UUID, ["pattern", 1])
            await write_helper(client, WRITE_UUID, [204,153,255,10,20], pkgidx=4)




if __name__ == "__main__":
   print("scan_devices")
   asyncio.get_event_loop().run_until_complete(scan_devices())
   print("scan_services")
   asyncio.get_event_loop().run_until_complete(scan_services())
   print("write_all")
   asyncio.get_event_loop().run_until_complete(write_all())
