#
# Module: rwc_nst.py
#
# Function:
#     Set up an RWC5020B LoRaWAN tester to send non-signaling (NST) FSK
#     frames for radio-bench-test.ino, and put it back afterwards.
#
# Copyright and License:
#     Please see accompanying LICENSE file.
#
# Author:
#     Terry Moore, MCCI Corporation    October 2026
#
# Usage:
#     Uses rwclib from mcci-catena/rwc-python-api. Run it from that
#     repository's examples/ directory (rwclib writes its log to ../logs/),
#     with this file copied or linked there:
#
#     python rwc_nst.py --address <tester-ip> --port <port> setup
#     python rwc_nst.py --address <tester-ip> --port <port> run
#     python rwc_nst.py --address <tester-ip> --port <port> restore
#     python rwc_nst.py --address <tester-ip> --port <port> cmd 'READ:NST:TX:STATUS?'
#
#     setup saves the settings it changes to rwc_nst_saved.json; restore
#     puts them back and restarts the tester's link. The FSK frames are
#     50 kbps, +/-25 kHz, sync word C1 94 C1, PN9 whitening, inverted
#     CCITT CRC, carrying the tester's NST payload (16 bytes by default).
#
#     Notes on rwclib: rf_settxfrequency() accepts whole MHz only, and
#     nst_tx_setsyncword() checks its value against 1..8, so this script
#     sends the SCPI commands itself.
#

import argparse
import json
import sys

sys.path.insert(0, '..')
from rwclib.cRWC5020x import RWCTesterApi             # noqa: E402
from rwclib.cRWCSerialSetup import RwcSerialSetup      # noqa: E402

SAVEFILE = 'rwc_nst_saved.json'

# (query, set-command prefix) for each setting that setup changes
SAVED = [
    ('READ:TESTER_MODE?',           'CONF:TESTER_MODE'),
    ('READ:NST:TX:MODULATION?',     'CONF:NST:TX:MODULATION'),
    ('READ:RF:TX_FREQ?',            'CONF:RF:TX_FREQ'),
    ('READ:NST:TX:DATA_RATE?',      'CONF:NST:TX:DATA_RATE'),
    ('READ:NST:TX:FM_DEVIATION?',   'CONF:NST:TX:FM_DEVIATION'),
    ('READ:NST:TX:SYNC_WORD_SIZE?', 'CONF:NST:TX:SYNC_WORD_SIZE'),
    ('READ:RF:TX_POW?',             'CONF:RF:TX_POW'),
    ('READ:NST:TX:REPEAT_NUM?',     'CONF:NST:TX:REPEAT_NUM'),
    ('READ:NST:TX:PACKET_INTERVAL?', 'CONF:NST:TX:PACKET_INTERVAL'),
]

SETUP = [
    'EXEC:LINK:STOP',
    'CONF:TESTER_MODE NST_TX',
    'CONF:NST:TX:MODULATION FSK',
    'CONF:RF:TX_FREQ 869.525',
    'CONF:NST:TX:DATA_RATE 50',
    'CONF:NST:TX:FM_DEVIATION 25',
    'CONF:NST:TX:SYNC_WORD_SIZE 3',
    'CONF:RF:TX_POW -40',
    'CONF:NST:TX:REPEAT_NUM 20',
    'CONF:NST:TX:PACKET_INTERVAL 0.5',
]

STATUS = [q for q, _ in SAVED] + [
    'READ:NST:TX:STATUS?', 'READ:NST:TX:SYNC_WORD?',
    'READ:NST:TX:PAYLOAD_SIZE?', 'READ:NST:TX:PAYLOAD?',
]


def main():
    ap = argparse.ArgumentParser(description='RWC5020B NST FSK setup for radio-bench-test')
    ap.add_argument('--address', required=True, help='tester IP address')
    ap.add_argument('--port', required=True, help='tester remote port')
    ap.add_argument('action', choices=['setup', 'run', 'stop', 'status', 'restore', 'cmd'])
    ap.add_argument('commands', nargs='*', help='SCPI commands for "cmd"')
    args = ap.parse_args()

    tester = RWCTesterApi(args.port, args.address)
    tester.open_port()

    def q(cmd):
        r = RwcSerialSetup.transceive(tester, cmd + '\n')
        print(f'{cmd:40s} -> {r!r}')
        return r

    try:
        if args.action == 'setup':
            saved = {setcmd: q(query) for query, setcmd in SAVED}
            with open(SAVEFILE, 'w') as f:
                json.dump(saved, f, indent=2)
            for c in SETUP:
                q(c)
        elif args.action == 'run':
            q('EXEC:NST:TX:RUN')
        elif args.action == 'stop':
            q('EXEC:NST:TX:STOP')
        elif args.action == 'restore':
            with open(SAVEFILE) as f:
                saved = json.load(f)
            q('EXEC:NST:TX:STOP')
            # set the tester mode last, then restart the link
            mode = saved.pop('CONF:TESTER_MODE', None)
            for setcmd, value in saved.items():
                q(f'{setcmd} {value}')
            if mode:
                q(f'CONF:TESTER_MODE {mode}')
            q('EXEC:LINK:RUN')
        elif args.action == 'cmd':
            for c in args.commands:
                q(c)
        if args.action != 'cmd':
            for c in STATUS:
                q(c)
    finally:
        tester.close_port()


if __name__ == '__main__':
    main()
