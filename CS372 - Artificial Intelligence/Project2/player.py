from enum import Enum

class Player(Enum):
    MAX = 1
    MIN = -1

    def other_player(self):
        if self == Player.MIN:
            return Player.MAX
        else:
            return Player.MIN
