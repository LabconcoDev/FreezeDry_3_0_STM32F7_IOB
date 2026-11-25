import argparse
import struct
import sys
import time
import serial

SOF_L = 0x4C
SOF_C = 0x43

# Classes
CLASS_READ  = 0x01
CLASS_WRITE = 0x02
CLASS_PUSH  = 0x03
CLASS_ACK   = 0x04

# Types
TYPE_SENSORS    = 0x01
TYPE_CONTROL    = 0x02
TYPE_ALERT      = 0x03
TYPE_SYSTEM     = 0x04
TYPE_SETTINGS   = 0x05
TYPE_DIAGNOSTIC = 0x06

# Sensor Actions
SENSORS = {
    "SENSOR_COLLECTOR_TEMP":                         0x01,
    "SENSOR_AMBIENT_TEMPERATURE":                    0x02,
    "SENSOR_LEXSOL_TEMP":                            0x03,
    "SENSOR_FIRST_STAGE_HEAT_EXCHANGER_OUTLET_TEMP": 0x04,
    "SENSOR_SECOND_STAGE_HEAT_EXCHANGER_OUTLET_TEMP":0x05,
    "SENSOR_FIRST_STAGE_SUCTION_TEMP":               0x06,
    "SENSOR_SECOND_STAGE_SUCTION_TEMP":              0x07,
    "SENSOR_SHELL_FREEZER_TEMP_SENSOR":              0x08,
    "SENSOR_MINI_CHAMBER_TEMP":                      0x09,
    "SENSOR_SHELF1_TEMP":                            0x0A,
    "SENSOR_SHELF1_SAMPLE_TEMP":                     0x0F,
    "SENSOR_SYSTEM_VAC":                             0x14,
    "SENSOR_SAMPLE_VAC1":                            0x15,
    "SENSOR_SAMPLE_VAC2":                            0x16,
    "SENSOR_SAMPLE_VAC3":                            0x17,
    "SENSOR_SAMPLE_VAC4":                            0x18,
    "SENSOR_SAMPLE_VAC5":                            0x19,
    "SENSOR_SAMPLE_VAC6":                            0x1A,
    "SENSOR_VACUUM_END_POINT_ADDON1":                0x1C,
    "SENSOR_VACUUM_END_POINT_ADDON2":                0x1D,
    "SENSOR_HARDWARE_DETECTION_IOB":                 0x1F,
}

# Control Actions
CONTROLS = {
    "CONTROL_REFRIG_RLY":          0x01,
    "CONTROL_VACUUM_RLY":          0x02,
    "CONTROL_PURGE_RLY":           0x03,
    "CONTROL_SHELL_FREEZER_RLY":   0x04,
    "CONTROL_DEFROST_RLY":         0x05,
    "CONTROL_VACUUM_VENT_VALVE":   0x06,
    "CONTROL_MINI_CHAMBER_HEATER": 0x07,
    "CONTROL_MANUAL_SHELF_HEAT":   0x08,
    "CONTROL_AUTO_MODE":           0x09,
    "CONTROL_PROGRAM_SEGMENTS":    0x0A,
    "CONTROL_PROGRAM_RUN":         0x0B,
    "CONTROL_PROGRAM_STATUS":      0x0C,
}


class FDProtocolClient:
    def __init__(self, port: str, baud: int = 115200, timeout: float = 0.2):
        self.ser = serial.Serial(port, baud, timeout=timeout)
        self.ser.reset_input_buffer()

    def close(self):
        self.ser.close()

    @staticmethod
    def calc_checksum(payload: bytes) -> int:
        return (len(payload) + sum(payload)) & 0xFFFF

    def build_frame(self, cls: int, msg_type: int, action: int, data: bytes = b"") -> bytes:
        payload = bytes([cls, msg_type, action]) + data
        csum = self.calc_checksum(payload)
        return bytes([SOF_L, SOF_C, len(payload)]) + payload + struct.pack("<H", csum)

    def read_frame(self, timeout_s: float = 1.5):
        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            b = self.ser.read(1)
            if not b or b[0] != SOF_L:
                continue
            b2 = self.ser.read(1)
            if not b2 or b2[0] != SOF_C:
                continue
            len_b = self.ser.read(1)
            if not len_b:
                continue
            plen = len_b[0]
            rest = self.ser.read(plen + 2)
            if len(rest) != plen + 2:
                continue
            payload = rest[:plen]
            rx_csum = struct.unpack("<H", rest[plen:])[0]
            if rx_csum != self.calc_checksum(payload):
                print(f"  [WARN] Bad checksum on RX: {rest.hex(' ')}")
                continue
            return payload[0], payload[1], payload[2], payload[3:]
        return None

    def send_and_expect(self, name: str, cls: int, msg_type: int, action: int,
                        data: bytes = b"", expect_ack: bool = True,
                        expect_push: bool = False, push_len: int | None = None,
                        decoder=None):
        frame = self.build_frame(cls, msg_type, action, data)
        self.ser.reset_input_buffer()
        self.ser.write(frame)

        got_ack = not expect_ack
        got_push = not expect_push
        push_data = b""

        deadline = time.monotonic() + 1.5
        while time.monotonic() < deadline and not (got_ack and got_push):
            pkt = self.read_frame(timeout_s=max(0.1, deadline - time.monotonic()))
            if pkt is None:
                break
            r_cls, r_type, r_act, r_data = pkt

            if r_cls == CLASS_ACK and r_type == msg_type and r_act == action:
                got_ack = True
            elif r_cls == CLASS_PUSH and r_type == msg_type and r_act == action:
                got_push = True
                push_data = r_data

        ok = got_ack and got_push
        if ok and expect_push and push_len is not None and len(push_data) != push_len:
            ok = False
            detail = f"Expected {push_len}B PUSH, got {len(push_data)}B ({push_data.hex(' ')})"
        elif ok and expect_push:
            detail = decoder(push_data) if decoder else f"PUSH: {push_data.hex(' ')}"
        elif ok:
            detail = "ACK OK"
        else:
            detail = f"TIMEOUT (ack={got_ack}, push={got_push})"

        status = "PASS" if ok else "FAIL"
        print(f"[{status}] {name:<50} | {detail}")
        return ok


def decode_temp(data: bytes) -> str:
    val = struct.unpack("<h", data)[0]
    if val == 0x7FFF:
        return "0x7FFF (stale/uninitialized)"
    return f"{val / 10.0:.1f} C"


def decode_vac(data: bytes) -> str:
    val = struct.unpack("<H", data)[0]
    if val == 0xFFFF:
        return "0xFFFF (stale/uninitialized)"
    return f"{val / 1000.0:.3f} mbar"


def decode_hw_iob(data: bytes) -> str:
    return f"opt={data[0]}, valves=0x{data[1]:02X}, epd=0x{data[2]:02X}"


def run_fd_tests(client: FDProtocolClient):
    results = []
    print("\n=== FD SENSORS (READ -> ACK + PUSH) ===")
    fd_temp_sensors = [
        "SENSOR_COLLECTOR_TEMP",
        "SENSOR_AMBIENT_TEMPERATURE",
        "SENSOR_FIRST_STAGE_HEAT_EXCHANGER_OUTLET_TEMP",
        "SENSOR_SECOND_STAGE_HEAT_EXCHANGER_OUTLET_TEMP",
        "SENSOR_FIRST_STAGE_SUCTION_TEMP",
        "SENSOR_SECOND_STAGE_SUCTION_TEMP",
        "SENSOR_SHELL_FREEZER_TEMP_SENSOR",
        "SENSOR_MINI_CHAMBER_TEMP",
    ]
    for s in fd_temp_sensors:
        results.append(client.send_and_expect(
            f"READ {s}", CLASS_READ, TYPE_SENSORS, SENSORS[s],
            expect_ack=True, expect_push=True, push_len=2, decoder=decode_temp
        ))

    fd_vac_sensors = [
        "SENSOR_SYSTEM_VAC",
        "SENSOR_SAMPLE_VAC1", "SENSOR_SAMPLE_VAC2", "SENSOR_SAMPLE_VAC3",
        "SENSOR_SAMPLE_VAC4", "SENSOR_SAMPLE_VAC5", "SENSOR_SAMPLE_VAC6",
        "SENSOR_VACUUM_END_POINT_ADDON1", "SENSOR_VACUUM_END_POINT_ADDON2",
    ]
    for s in fd_vac_sensors:
        results.append(client.send_and_expect(
            f"READ {s}", CLASS_READ, TYPE_SENSORS, SENSORS[s],
            expect_ack=True, expect_push=True, push_len=2, decoder=decode_vac
        ))

    results.append(client.send_and_expect(
        "READ SENSOR_HARDWARE_DETECTION_IOB", CLASS_READ, TYPE_SENSORS,
        SENSORS["SENSOR_HARDWARE_DETECTION_IOB"],
        expect_ack=True, expect_push=True, push_len=3, decoder=decode_hw_iob
    ))

    print("\n=== FD CONTROLS (WRITE -> ACK, READ -> ACK + PUSH) ===")
    for c in ["CONTROL_REFRIG_RLY", "CONTROL_PURGE_RLY", "CONTROL_SHELL_FREEZER_RLY",
              "CONTROL_DEFROST_RLY", "CONTROL_VACUUM_VENT_VALVE"]:
        results.append(client.send_and_expect(
            f"WRITE {c} (ON)", CLASS_WRITE, TYPE_CONTROL, CONTROLS[c], b"\x01"
        ))
        results.append(client.send_and_expect(
            f"WRITE {c} (OFF)", CLASS_WRITE, TYPE_CONTROL, CONTROLS[c], b"\x00"
        ))
        results.append(client.send_and_expect(
            f"READ {c}", CLASS_READ, TYPE_CONTROL, CONTROLS[c],
            expect_ack=True, expect_push=True, push_len=1
        ))

    # 3-byte Vacuum Relay: flags (ON + VacProtect) + 0.120 mbar (120)
    vac_payload = struct.pack("<BH", 0x11, 120)
    results.append(client.send_and_expect(
        "WRITE CONTROL_VACUUM_RLY (0.120 mbar)", CLASS_WRITE, TYPE_CONTROL,
        CONTROLS["CONTROL_VACUUM_RLY"], vac_payload
    ))
    results.append(client.send_and_expect(
        "READ CONTROL_VACUUM_RLY", CLASS_READ, TYPE_CONTROL,
        CONTROLS["CONTROL_VACUUM_RLY"], expect_ack=True, expect_push=True, push_len=1
    ))

    # 3-byte Mini Chamber Heater: ON + 25.0 C (250)
    mc_payload = struct.pack("<Bh", 0x01, 250)
    results.append(client.send_and_expect(
        "WRITE CONTROL_MINI_CHAMBER_HEATER (25.0 C)", CLASS_WRITE, TYPE_CONTROL,
        CONTROLS["CONTROL_MINI_CHAMBER_HEATER"], mc_payload
    ))
    results.append(client.send_and_expect(
        "READ CONTROL_MINI_CHAMBER_HEATER", CLASS_READ, TYPE_CONTROL,
        CONTROLS["CONTROL_MINI_CHAMBER_HEATER"], expect_ack=True, expect_push=True, push_len=1
    ))

    results.append(client.send_and_expect(
        "WRITE CONTROL_AUTO_MODE (ON)", CLASS_WRITE, TYPE_CONTROL,
        CONTROLS["CONTROL_AUTO_MODE"], b"\x01"
    ))
    results.append(client.send_and_expect(
        "WRITE CONTROL_AUTO_MODE (OFF)", CLASS_WRITE, TYPE_CONTROL,
        CONTROLS["CONTROL_AUTO_MODE"], b"\x00"
    ))
    return results


def run_triad_tests(client: FDProtocolClient):
    results = []
    print("\n=== TRIAD EXTRA SENSORS ===")
    for s in ["SENSOR_LEXSOL_TEMP", "SENSOR_SHELF1_TEMP", "SENSOR_SHELF1_SAMPLE_TEMP"]:
        results.append(client.send_and_expect(
            f"READ {s}", CLASS_READ, TYPE_SENSORS, SENSORS[s],
            expect_ack=True, expect_push=True, push_len=2, decoder=decode_temp
        ))

    print("\n=== TRIAD CONTROLS & PROGRAM ===")
    shelf_payload = struct.pack("<Bh", 0x01, -200)  # ON, -20.0 C
    results.append(client.send_and_expect(
        "WRITE CONTROL_MANUAL_SHELF_HEAT (-20.0 C)", CLASS_WRITE, TYPE_CONTROL,
        CONTROLS["CONTROL_MANUAL_SHELF_HEAT"], shelf_payload
    ))

    # 10-byte segment: seg=1, flags=0x03, temp=-30.0C (-300), ramp=1.50 (150), vac=0.100 (100), hold=60m
    seg_payload = struct.pack("<BBhHHH", 1, 0x03, -300, 150, 100, 60)
    results.append(client.send_and_expect(
        "WRITE CONTROL_PROGRAM_SEGMENTS (Seg 1)", CLASS_WRITE, TYPE_CONTROL,
        CONTROLS["CONTROL_PROGRAM_SEGMENTS"], seg_payload
    ))
    results.append(client.send_and_expect(
        "WRITE CONTROL_PROGRAM_RUN (START)", CLASS_WRITE, TYPE_CONTROL,
        CONTROLS["CONTROL_PROGRAM_RUN"], b"\x01"
    ))
    results.append(client.send_and_expect(
        "READ CONTROL_PROGRAM_STATUS", CLASS_READ, TYPE_CONTROL,
        CONTROLS["CONTROL_PROGRAM_STATUS"], expect_ack=True, expect_push=True, push_len=4
    ))
    results.append(client.send_and_expect(
        "WRITE CONTROL_PROGRAM_RUN (STOP)", CLASS_WRITE, TYPE_CONTROL,
        CONTROLS["CONTROL_PROGRAM_RUN"], b"\x00"
    ))
    return results


def main():
    parser = argparse.ArgumentParser(description="Freeze Dry 3.0 ICD UART Test Suite")
    parser.add_argument("--port", required=True, help="COM port (e.g. COM5 or /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--mode", choices=["fd", "triad", "all"], default="fd")
    args = parser.parse_args()

    client = FDProtocolClient(args.port, args.baud)
    try:
        results = []
        if args.mode in ("fd", "all"):
            results.extend(run_fd_tests(client))
        if args.mode in ("triad", "all"):
            results.extend(run_triad_tests(client))

        passed = sum(1 for r in results if r)
        total = len(results)
        print(f"\n=== SUMMARY: {passed}/{total} PASSED ===")
        sys.exit(0 if passed == total else 1)
    finally:
        client.close()


if __name__ == "__main__":
    main()