from enum import Enum

class GameState(Enum):
    IN_PROGRESS = 10
    MAX_WIN = 1
    MIN_WIN = -1
    TIE = 0
