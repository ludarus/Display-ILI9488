# will parse a trc file found in logs/ and output the according can messages
import can
import time
import argparse
import os
import sys


class CAN_message:
    # constructor
    def __init__(
        self,
        msgNum: int,
        msgTime: float,
        msgType: str,
        msgId: int,
        msgDlc: int,
        msgData: list,
    ):
        self.num = msgNum
        self.time = msgTime
        self.receieved = msgType
        self.id = msgId
        self.dlc = msgDlc
        self.data = msgData


def parse_file(filePath: str) -> list[CAN_message]:
    """parses a .trc file and converts to a list of can messages"""

    # opening file
    with open(filePath, "rt") as f:
        file = f.read()

    # splitting the file into lines
    lines = file.splitlines()

    # creating messages array
    messages: list[CAN_message] = []

    # removing the header from the trace file
    for i in range(14):
        lines.pop(0)

    # parsing each line
    for line in lines:
        # splitting the line into words
        values = line.split()

        # checking if message isn't valid to send
        if values[2] != "Rx":
            continue

        # parsing message values
        data: int = []

        # parsing data
        for i in range(5, len(values)):
            data.append(int(values[i], 16))

        # creating a message object out of the parsed word
        msg = CAN_message(
            msgNum=int(values[0][:-1]),
            msgTime=float(values[1]),
            msgType=values[2],
            msgId=int(values[3], 16),
            msgDlc=int(values[4]),
            msgData=data,
        )

        # appending to list
        messages.append(msg)

    return messages


def send_messages(messages: list[CAN_message]) -> None:
    """sends list of can messages to a CAN device"""

    bus = can.interface.Bus(channel="can0", interface="socketcan")

    # first message
    bus.send(
        can.Message(
            arbitration_id=messages[0].id,
            is_extended_id=False,
            data=messages[0].data,
        )
    )

    # sending all other messages
    # stress testing
    # while True:
    for msgIdx in range(1, len(messages)):

        # waiting the specified delay. will have some small time drift but its fine for this application
        time.sleep((messages[msgIdx].time - messages[msgIdx - 1].time) / 1000.0)

        print(
            f"sending msg with id = {hex(messages[msgIdx].id)}, data = {messages[msgIdx].data}"
        )

        # sending the message
        bus.send(
            can.Message(
                arbitration_id=messages[msgIdx].id,
                is_extended_id=False,
                data=messages[msgIdx].data,
            )
        )

    # closing bus after use
    bus.shutdown()


def main():
    # adding command line argument for the desired input file
    parser = argparse.ArgumentParser()
    parser.add_argument("inputFile", help="a .trc file to emulate")

    args = parser.parse_args()

    # checking if input is a file
    if not os.path.isfile(args.inputFile):
        print(f"ERROR: File not found: {args.inputFile}", file=sys.stderr)
        sys.exit(1)

    # sending the messages
    send_messages(parse_file(args.inputFile))


if __name__ == "__main__":
    main()
