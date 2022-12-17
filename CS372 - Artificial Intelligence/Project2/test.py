from board import Board
from player import Player
from gamestate import GameState
from time import time
import math


def get_points(list):
    count = 1
    points = 0
    space_before = False
    for i in range(len(list) - 1):
        if count == 1:
            space_before = i > 0 and list[i-1] == 0
        if list[i] == list[i + 1]:
            count += 1
        else:
            space_after = list[i+1] == 0
            if space_after or space_before:
                if count == 2:
                    points += 30 * list[i] if (space_before and space_after) else 10 * list[i]
                if count == 3:
                    points += 5000 if (space_before and space_after) else 100 * list[i]
            count = 1
    space_after = False
    if space_before:
        if count == 2:
            points += 10 * list[i]
        if count == 3:
            points += 100 * list[i]
    count = 1
    return points
        
def get_ne_diagonals(board):
    diagonals = []
    for r in range(board.get_rows()):
        c = 0
        diagonal = [board.board[r][c]]
        next_r = r + 1
        while next_r < board.get_rows() and c < board.get_cols() - 1:
            diagonal.append(board.board[next_r][c + 1])
            c += 1
            next_r += 1
        diagonals.append(diagonal)
    for c in range(1, board.get_cols()):
        r = 0
        diagonal = [board.board[r][c]]
        next_c = c + 1
        while next_c < board.get_cols() and r < board.get_rows() - 1:
            diagonal.append(board.board[r + 1][next_c])
            r += 1
            next_c += 1
        diagonals.append(diagonal)
    return diagonals

def get_nw_diagonals(board):
    diagonals = []
    for r in range(board.get_rows()):
        c = 0
        diagonal = [board.board[r][c]]
        next_r = r - 1
        while next_r >= 0 and c < board.get_cols() - 1:
            diagonal.append(board.board[next_r][c + 1])
            c += 1
            next_r -= 1
        diagonals.append(diagonal)
    for c in range(1, board.get_cols()):
        r = board.get_rows() - 1
        diagonal = [board.board[r][c]]
        next_c = c + 1
        while next_c < board.get_cols() and r > 0:
            diagonal.append(board.board[r - 1][next_c])
            r -= 1
            next_c += 1
        diagonals.append(diagonal)
    return diagonals

def get_columns(board):
    columns = []
    for c in range(board.get_cols()):
        column = []
        for r in range(board.get_rows()):
            column.append(board.board[r][c])
        columns.append(column)
    return columns

board = Board(6, 6, 3)
move = 0
while 0 <= move < board.get_cols():
    print(board.to_2d_string())
    print(get_ne_diagonals(board))
    print(get_nw_diagonals(board))
    print(get_columns(board))
    points = 0
    for row in board.board:
        points += get_points(row)
    for col in get_columns(board):
        points += get_points(col)
    for ne_diagonals in get_ne_diagonals(board):
        points += get_points(ne_diagonals)
    for nw_diagonals in get_nw_diagonals(board):
        points += get_points(nw_diagonals)
    print(points)
    move = int(input("move: "))
    board = board.make_move(move)