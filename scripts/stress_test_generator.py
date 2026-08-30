# stress_test_generator.py
# Luke Fadel 2026

# script to stress test, purposely overflow and stall the display via usb CAN messages
# stress testing parameters are tweakable at the bottom of this file


import can
import time
import random

# constants
DISPLAY_WIDTH = 480
CHAR_WIDTH = 32
FONT_SIZE = 96


# lists of object numbers for each type
# output of generate_object_table.py

# using tuples for immutability
# background objNums
backgroundIds = (1, 4, 5, 6, 135, 136, 147, 148, 149)

# text objNums with their x location
textIds = (
    (58, 0),
    (59, 240),
    (60, 408),
    (61, 0),
    (62, 264),
    (63, 0),
    (64, 0),
    (65, 0),
    (66, 0),
    (67, 48),
    (68, 48),
    (69, 48),
    (70, 48),
    (71, 48),
    (72, 288),
    (73, 288),
    (74, 288),
    (75, 288),
    (76, 288),
    (89, 0),
    (137, 144),
    (146, 96),
)

# image objNums
imageIds = (
    2,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24,
    25,
    26,
    27,
    28,
    29,
    30,
    31,
    32,
    33,
    34,
    35,
    36,
    37,
    38,
    39,
    40,
    41,
    42,
    43,
    44,
    45,
    46,
    47,
    48,
    49,
    50,
    51,
    52,
    53,
    54,
    55,
    56,
    57,
    77,
    78,
    79,
    80,
    81,
    82,
    83,
    84,
    85,
    86,
    87,
    88,
    90,
    91,
    92,
    93,
    94,
    97,
    98,
    99,
    100,
    101,
    102,
    103,
    104,
    105,
    106,
    107,
    108,
    109,
    110,
    111,
    112,
    113,
    114,
    115,
    116,
    117,
    118,
    119,
    120,
    121,
    122,
    123,
    124,
    125,
    126,
    127,
    128,
    129,
    130,
    131,
    132,
    133,
    134,
    138,
    139,
    140,
    141,
    142,
    143,
    144,
    145,
)

# group objNums
groupIds = (95, 96)


# dataclass to hold can message info
class CanMsg:
    def __init__(self, id: int, timestamp: int, data: list[int]):
        self.id = id
        self.timestamp = timestamp
        # 8 entry
        self.data = data

    # return self string override
    def __repr__(self):
        return f"""
id = 0x{self.id:X},
timestamp = {self.timestamp},
data = {self.data}
"""


# parent generator class
class MsgGenerator:
    def __init__(self, interval: int, count: int = None):
        self.interval = interval
        self.count = count

    # default function generates random message
    def createData(self, currentTime: int) -> list[CanMsg]:
        data = []
        for i in range(random.randint(0, 7)):
            data.append(random.randint(0, 0xFF))

        return [CanMsg(random.randint(0, 2047), currentTime + self.interval, data)]

    # returns list of messages with correct timestamps
    def createList(self) -> list[CanMsg]:
        msgList: list[CanMsg] = []
        for i in range(self.count):
            msgList += self.createData(i * self.interval)

        return msgList


# inherited child classes ---
# generates bg messages
class BackgroundGenerator(MsgGenerator):
    def createData(self, currentTime: int) -> list[CanMsg]:
        # select random background
        objNum = backgroundIds[random.randint(0, len(backgroundIds) - 1)]

        # data = LSB, MSB
        data = [objNum & 0xFF, objNum >> 8]

        return [CanMsg(0x418, currentTime + self.interval, data)]


# generates text messages
class TextGenerator(MsgGenerator):
    def createData(self, currentTime: int) -> list[CanMsg]:
        idx = random.randint(0, len(textIds) - 1)

        # select max number of chars for current objNum by using x value
        numberOfChars = (480 - textIds[idx][1]) // CHAR_WIDTH
        print("num of chars = ", numberOfChars)
        objNum = textIds[idx][0]

        messages = []
        data = [
            0,
            numberOfChars,
            objNum & 0xFF,
            objNum >> 8,
        ]

        # fill up messages with the rest of the characters
        for i in range(numberOfChars):
            # add random ascii character
            data.append(random.randint(0, FONT_SIZE - 1) + 32)

            if len(data) >= 8:
                messages.append(CanMsg(0x420, currentTime + self.interval, data.copy()))
                data.clear()

        messages.append(CanMsg(0x420, currentTime + self.interval, data.copy()))

        return messages


# generates image messages
class ImageGenerator(MsgGenerator):
    def createData(self, currentTime: int) -> list[CanMsg]:
        # select random background
        objNum = imageIds[random.randint(0, len(imageIds) - 1)]

        # data = LSB, MSB
        data = [objNum & 0xFF, objNum >> 8]

        return [CanMsg(0x428, currentTime + self.interval, data)]


# ??? unknown how to generate proper message for this
class GroupGenerator(MsgGenerator): ...


class VersionGenerator(MsgGenerator):
    def createData(self, currentTime: int) -> list[CanMsg]:

        # version request command
        return [CanMsg(0x438, currentTime + self.interval, [])]


class BrightnessGenerator(MsgGenerator):
    def createData(self, currentTime: int) -> list[CanMsg]:
        flag = random.randint(1, 2)

        # 5% chance to reset
        if random.randint(1, 20) == 17:
            flag = 3

        # random brightness value
        return [CanMsg(0x448, currentTime + self.interval, [flag])]


class AlarmGenerator(MsgGenerator):
    def createData(self, currentTime: int) -> list[CanMsg]:

        # random alarm
        return [
            CanMsg(
                0x450,
                currentTime + self.interval,
                [random.randint(0, 1), random.randint(0, 7)],  # duty cycle, frequency
            )
        ]


class FailureGenerator(MsgGenerator):
    def createData(self, currentTime: int) -> list[CanMsg]:
        # failure command
        return [CanMsg(0x440, currentTime + self.interval, [])]


def combine_generators(generators: list[MsgGenerator]) -> list[CanMsg]:
    # combines n generators to output a final sorted list of commands
    combinedMessages = []
    for generator in generators:
        combinedMessages += generator.createList()

    # sorting combined messages by timestamp
    sortedMessages = sorted(combinedMessages, key=lambda msg: msg.timestamp)

    return sortedMessages


def send_messages(messages: list[CanMsg]):
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
        time.sleep(
            (messages[msgIdx].timestamp - messages[msgIdx - 1].timestamp) / 1000.0
        )

        print(messages[msgIdx])

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


# -------------- main --------------

messages = combine_generators(
    [  # List of generators. Probably make some of the intervals prime
        # parameters: Generator(interval, number of messages)
        # MsgGenerator(1, 16000),
        # BackgroundGenerator(300, 1000),
        ImageGenerator(10, 878),
        # AlarmGenerator(50, 500),
        # BrightnessGenerator(100, 500),
        # FailureGenerator(1000, 5),
        TextGenerator(7, 1000),
        # VersionGenerator(10, 1000),
    ]
)

# sending messages
send_messages(messages)
