from enum import Enum


class Format(Enum):
    CHAR = 1
    SHORT = 2
    FLOAT = 3
    COMPLEX_CHAR = 4
    COMPLEX_FLOAT = 5
    COMPLEX_SHORT = 6
    COMPLEX_CHAR = 7


class AgcProfile(Enum):
    SLOW = ("Slow", 0.01, 0.0001, 600)
    FAST = ("Fast", 0.1, 0.001, 200)

    def __new__(cls, name: str, attack: float, decay: float, hangTime: int):
        obj = object.__new__(cls)
        obj._value_ = name
        obj.attack = attack
        obj.decay = decay
        obj.hangTime = hangTime
        return obj
