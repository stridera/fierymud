#!/usr/bin/python
import struct
from string import strip
from datetime import datetime

blocks = {}
pos = 0
with open('plrmail', 'rb') as f:
    while 1:
        block_data = f.read(4)
        if not block_data:
            break

        block_type = struct.unpack('<i', block_data)[0]
        data = f.read(100 - 4)

        # if block_type == -3:  # Deleted block
        #     continue

        if block_type == -1:  # Header Block
            nextBlock, fromID, toID, itemID, timestamp, msg = struct.unpack('iiiii76s', data)
            time_string = datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
            blocks[pos] = {
                'isHeader': True,
                'from': fromID,
                'nextBlock': nextBlock,
                'to': toID,
                'item': 'None' if itemID == 65535 else itemID,
                'timestamp': time_string,
                'msg': msg.rstrip('\0')
            }

        else:
            msg = struct.unpack('96s', data)[0].rstrip('\0')

            blocks[pos] = {
                'isHeader': False,
                'nextBlock': block_type,
                'msg': msg
            }

        # print(pos, block_type, blocks[pos]['msg'])
        pos += 100

for pos in blocks:
    trunc = -1
    block = blocks[pos]
    if block['isHeader']:
        msg = block['msg']
        nextBlock = block['nextBlock']
        while nextBlock != -2:
            if nextBlock in blocks:
                msg = msg + blocks[nextBlock]['msg']
                nextBlock = blocks[nextBlock]['nextBlock']
            else:
                trunc = nextBlock
                break

        print("From: {}\nTo: {}\nItem ID: {}\nTimestamp: {}\n\n{}\n\n".format(
            block['from'],
            block['to'],
            block['item'],
            block['timestamp'],
            msg
        ))
        if trunc != -1:
            print("ERROR: Message truncated.  Block {} not found".format(trunc))

        print('#' * 80)
