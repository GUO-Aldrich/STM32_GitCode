import serial
import serial.tools.list_ports
import sys
import threading
import argparse
import time
import msvcrt


COMMON_BAUDRATES = [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600]

SHELL_TOKENS = [b'>', b'help', b'shell', b'SHELL', b'Letter', b'reboot', b'root']

PROMPT_PATTERNS = [b'root:/$', b'root:/', b'> ', b'>']


def list_ports():
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("未检测到任何串口设备")
        return []
    print("\n可用串口列表:")
    print("-" * 50)
    for i, p in enumerate(ports):
        print(f"  [{i}] {p.device:10s} {p.description}")
    print("-" * 50)
    return ports


def detect_baudrate(port):
    print(f"\n正在扫描 {port} 的常用波特率 ...")
    print("-" * 50)

    results = []

    for br in COMMON_BAUDRATES:
        sys.stdout.write(f"  尝试 {br:>7d} ... ")
        sys.stdout.flush()
        matched = False
        sample = b''
        try:
            ser = serial.Serial(port, br, timeout=0.1)
            time.sleep(0.15)
            if ser.in_waiting:
                ser.read(ser.in_waiting)
            ser.write(b'\nhelp\n')
            time.sleep(0.35)
            if ser.in_waiting:
                sample = ser.read(ser.in_waiting)
            ser.close()
        except Exception as e:
            print(f"  错误: {e}")
            results.append((br, False, b''))
            continue

        sample_lower = sample.lower()
        for tok in SHELL_TOKENS:
            if tok.lower() in sample_lower:
                matched = True
                break

        results.append((br, matched, sample))
        if matched:
            preview = sample[:60].replace(b'\r', b'\\r').replace(b'\n', b'\\n')
            print(f"  匹配成功!  响应预览: {preview.decode('ascii', errors='replace')}")
        else:
            print(f"  无响应")

    print("-" * 50)

    matched_brs = [r for r in results if r[1]]
    if not matched_brs:
        print("未能自动检测到波特率，请手动指定 (-b)")
        return None

    chosen = matched_brs[-1][0]
    print(f"检测到 {len(matched_brs)} 个可能匹配的波特率: ", end='')
    print(', '.join(str(r[0]) for r in matched_brs))
    print(f"选择最高的那个: {chosen}")
    return chosen


def read_loop(ser, stop_event):
    while not stop_event.is_set():
        try:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                try:
                    print(data.decode('utf-8', errors='replace'), end='', flush=True)
                except Exception:
                    print(data, end='', flush=True)
        except serial.SerialException:
            print("\n串口连接已断开")
            break
        except Exception:
            pass
        time.sleep(0.01)


def wait_prompt(ser, timeout=3.0):
    buf = b''
    deadline = time.time() + timeout
    probe_sent = False
    while time.time() < deadline:
        if not probe_sent:
            ser.write(b'\n')
            probe_sent = True
        chunk = ser.read(1)
        if chunk:
            buf += chunk
            for p in PROMPT_PATTERNS:
                if p in buf:
                    return True
        else:
            if probe_sent and not buf:
                probe_sent = False
    return False


def main():
    parser = argparse.ArgumentParser(description="STM32 Shell 终端工具")
    parser.add_argument('-p', '--port', type=str, help='串口名称 (如 COM3)')
    parser.add_argument('-b', '--baudrate', type=int, help='波特率 (不指定则自动扫描)')
    parser.add_argument('-l', '--list', action='store_true', help='列出所有可用串口')
    parser.add_argument('-n', '--no-reboot', action='store_true', help='连接后不自动发送 reboot')
    args = parser.parse_args()

    if args.list:
        list_ports()
        return

    ports = list_ports()
    if not ports:
        sys.exit(1)

    port = args.port
    if port:
        matched = [p for p in ports if p.device.upper() == port.upper()]
        if not matched:
            print(f"警告: 当前未检测到 {port}，将继续尝试连接...")
    else:
        if len(ports) == 1:
            port = ports[0].device
            print(f"仅检测到一个串口，自动选择: {port}  ({ports[0].description})")
        else:
            try:
                raw = input(f"请输入要使用的串口编号 [0-{len(ports)-1}] (默认0): ").strip()
                idx = int(raw) if raw else 0
                if idx < 0 or idx >= len(ports):
                    print("编号无效")
                    sys.exit(1)
                port = ports[idx].device
            except (ValueError, EOFError):
                port = ports[0].device

    baudrate = args.baudrate
    if not baudrate:
        baudrate = detect_baudrate(port)
        if not baudrate:
            sys.exit(1)

    print(f"\n正在连接 {port} @ {baudrate} ...")

    try:
        ser = serial.Serial(port, baudrate, timeout=0.1)
    except Exception as e:
        print(f"连接失败: {e}")
        sys.exit(1)

    time.sleep(0.15)
    if ser.in_waiting:
        ser.read(ser.in_waiting)

    print(f"\n已连接!  按 Ctrl+] 退出\n")

    if not args.no_reboot:
        print("等待 shell 就绪 ...")
        wait_prompt(ser)
        time.sleep(0.05)
        ser.write(b'reboot\n')

    stop_event = threading.Event()
    t = threading.Thread(target=read_loop, args=(ser, stop_event), daemon=True)
    t.start()

    SCANCODE_MAP = {
        'H': b'\x1b[A',
        'P': b'\x1b[B',
        'M': b'\x1b[C',
        'K': b'\x1b[D',
        'G': b'\x1b[H',
        'O': b'\x1b[F',
        'I': b'\x1b[5~',
        'Q': b'\x1b[6~',
        'R': b'\x1b[2~',
        'S': b'\x1b[3~',
    }

    try:
        while True:
            ch = msvcrt.getwch()
            if ch == '\x1d':
                print("\n正在退出...")
                break
            if ch == '\xe0':
                nxt = msvcrt.getwch()
                seq = SCANCODE_MAP.get(nxt)
                if seq:
                    ser.write(seq)
                continue
            if ch == '\x00':
                msvcrt.getwch()
                continue
            if ord(ch) < 0x80:
                ser.write(bytes([ord(ch)]))
    except KeyboardInterrupt:
        print("\n正在退出...")
    except EOFError:
        pass
    finally:
        stop_event.set()
        try:
            ser.close()
        except:
            pass
        print("已断开连接")


if __name__ == '__main__':
    main()