from distutils.log import debug
from board import Board
from player import Player
from gamestate import GameState
from time import time
import math

# Return ultility of final state
def utility(board):
    return int(board.get_game_state()._value_ * 10000 * board.get_rows() * board.get_cols() / board.get_number_of_moves())

# Return a list of possible actions
def actions(board):
    available_actions = []
    for i in range(board.get_cols()):
        if not board.is_column_full(i):
            available_actions.append(i)
    return available_actions

# Minimax algorithm for part A
def minimax_A(board, table):
    if board in table.keys():
        return table[board]
    elif board.get_game_state() != GameState.IN_PROGRESS:
        info = (utility(board), None)
        table[board] = info
        return info
    elif board.get_player_to_move_next() == Player.MAX:
        v = -math.inf
        best_move = None
        for action in actions(board):
            child_board = board.make_move(action)
            child_info = minimax_A(child_board, table)
            v2 = child_info[0]
            if v2 > v:
                v = v2
                best_move = action
        info = (v, best_move)
        table[board] = info
        return info
    elif board.get_player_to_move_next() == Player.MIN:
        v = math.inf
        best_move = None
        for action in actions(board):
            child_board = board.make_move(action)
            child_info = minimax_A(child_board, table)
            v2 = child_info[0]
            if v2 < v:
                v = v2
                best_move = action
        info = (v, best_move)
        table[board] = info
        return info

# Number of times the tree is pruned
prunings = 0

# Minimax algorithm for part B
def minimax_B(board, alpha, beta, table):
    global prunings
    if board in table.keys():
        return table[board]
    elif board.get_game_state() != GameState.IN_PROGRESS:
        info = (utility(board), None)
        table[board] = info
        return info
    elif board.get_player_to_move_next() == Player.MAX:
        v = -math.inf
        best_move = None
        for action in actions(board):
            child_board = board.make_move(action)
            child_info = minimax_B(child_board, alpha, beta, table)
            v2 = child_info[0]
            if v2 > v:
                v = v2
                best_move = action
                alpha = max(alpha, v)
            if v >= beta:
                prunings += 1
                return (v, best_move)
        info = (v, best_move)
        table[board] = info
        return info
    elif board.get_player_to_move_next() == Player.MIN:
        v = math.inf
        best_move = None
        for action in actions(board):
            child_board = board.make_move(action)
            child_info = minimax_B(child_board, alpha, beta, table)
            v2 = child_info[0]
            if v2 < v:
                v = v2
                best_move = action
                beta = min(beta, v)
            if v <= alpha:
                prunings += 1
                return (v, best_move)
        info = (v, best_move)
        table[board] = info
        return info

# Minimax algorithm for part C
def minimax_C(board, alpha, beta, depth, max_depth, table):
    if board in table.keys():
        return table[board]
    elif board.get_game_state() != GameState.IN_PROGRESS:
        info = (utility(board), None)
        table[board] = info
        return info
    elif depth == max_depth:
        info  = (heuristic(board), None)
        table[board] = info
        return info
    elif board.get_player_to_move_next() == Player.MAX:
        v = -math.inf
        best_move = None
        for action in actions(board):
            child_board = board.make_move(action)
            child_info = minimax_C(child_board, alpha, beta, depth + 1, max_depth, table)
            v2 = child_info[0]
            if v2 > v:
                v = v2
                best_move = action
                alpha = max(alpha, v)
            if v >= beta:
                return (v, best_move)
        info = (v, best_move)
        table[board] = info
        return info
    elif board.get_player_to_move_next() == Player.MIN:
        v = math.inf
        best_move = None
        for action in actions(board):
            child_board = board.make_move(action)
            child_info = minimax_C(child_board, alpha, beta, depth + 1, max_depth, table)
            v2 = child_info[0]
            if v2 < v:
                v = v2
                best_move = action
                beta = min(beta, v)
            if v <= alpha:
                return (v, best_move)
        info = (v, best_move)
        table[board] = info
        return info

# Get the number of 3-in-a-rows and 2-in-a-rows in a 1D array, then calculate a heuristic value. 
# Space at either ends is taken into account.
def get_points(list):
    count = 1
    points = 0
    space_before = False
    for i in range(len(list) - 1):
        # Check if there's space before the first number in the streak
        if count == 1:
            space_before = i > 0 and list[i-1] == 0
        # Increment count if the streak continues
        if list[i] == list[i + 1]:
            count += 1
        # Add points if there is a streak
        else:
            space_after = list[i+1] == 0
            if space_after or space_before:
                if count == 2:
                    points += 30 * list[i] if (space_before and space_after) else 10 * list[i]
                if count == 3:
                    points += 2000 * list[i] if (space_before and space_after) else 100 * list[i]
            count = 1
    # Add points for the final streak, if it exists
    space_after = False
    if space_before:
        if count == 2:
            points += 10 * list[i]
        if count == 3:
            points += 100 * list[i]
    return points
            
# Get a list of all northeast diagonals of the boards. Each diagonal is represented by a 1D array
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

# Get a list of all northwest diagonals of the boards. Each diagonal is represented by a 1D array
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

# Get a list of all columns of the boards. Each diagonal is represented by a 1D array
def get_columns(board):
    columns = []
    for c in range(board.get_cols()):
        column = []
        for r in range(board.get_rows()):
            column.append(board.board[r][c])
        columns.append(column)
    return columns

# Heuristic function
def heuristic(board):
    h = 0
    for row in board.board:
        h += get_points(row)
    for col in get_columns(board):
        h += get_points(col)
    for ne_diagonals in get_ne_diagonals(board):
        h += get_points(ne_diagonals)
    for nw_diagonals in get_nw_diagonals(board):
        h += get_points(nw_diagonals)
    return h

# Play part C
def play_C(debugging, r, c, inarow):
    max_depth = int(input("Number of moves to look ahead (depth): "))
    first_player = int(input("Who plays first? 1=human, 2=computer: "))
    players = dict()
    if first_player == 1:
        players["MAX"] = "human"
        players["MIN"] = "computer"
    else:
        players["MAX"] = "computer"
        players["MIN"] = "human"
    while True:
        current_board = Board(r, c, inarow)
        while current_board.get_game_state() == GameState.IN_PROGRESS:
            table = dict()
            result = minimax_C(current_board, -math.inf, math.inf, 0, max_depth, table)
            if debugging:
                for key in table.keys():
                    print(key.to_2d_string())
                    print(table[key])
            print(current_board.to_2d_string())
            print("Minimax value for this state: %d, optimal move: %d" % (result[0], result[1]))
            next_player = "MAX" if current_board.get_player_to_move_next() == Player.MAX else "MIN"
            print("It is %s's turn!" % (next_player))
            if players[next_player] == "human":
                move = int(input("Enter move: "))
            else:
                move = table[current_board][1]
                print("Computer chooses move: %d" % (move))
            current_board = current_board.make_move(move)
        print("Game over!")
        print(current_board.to_2d_string())
        if current_board.get_game_state() != GameState.TIE:
            winner = "MAX" if current_board.get_game_state() == GameState.MAX_WIN else "MIN"
            print("The winner is %s (%s)" % (winner, players[winner]))
        else:
            print("Draw!")
        again = input("Play again? (y/n): ")
        if again == "n": break

# Play part B
def play_B(debugging, r, c, inarow):
    table = dict()
    board = Board(r, c, inarow)
    start = time()
    result = minimax_B(board, -math.inf, math.inf, table)
    end = time()
    if debugging:
        for key in table.keys():
            print(key.to_2d_string())
            print(table[key])
    print("Search completed in %.3f seconds." % (end - start))
    print("Transposition table has %d states." % (len(table.keys())))
    print("The tree was pruned %d times." % (prunings))
    if result[0] == 0:
        print("The game results in a draw with perfect play")
    elif result[0] > 0:
        print("First player has a guaranteed win with perfect play.")
    else:
        print("Second player has a guaranteed win with perfect play.")
    first_player = int(input("Who plays first? 1=human, 2=computer: "))
    players = dict()
    if first_player == 1:
        players["MAX"] = "human"
        players["MIN"] = "computer"
    else:
        players["MAX"] = "computer"
        players["MIN"] = "human"
    while True:
        current_board = Board(r, c, inarow)
        while current_board.get_game_state() == GameState.IN_PROGRESS:
            print(current_board.to_2d_string())
            print("Minimax value for this state: %d, optimal move: %d" % (table[current_board][0], table[current_board][1]))
            next_player = "MAX" if current_board.get_player_to_move_next() == Player.MAX else "MIN"
            print("It is %s's turn!" % (next_player))
            if players[next_player] == "human":
                move = int(input("Enter move: "))
                # Rerun if player chooses suboptimal move 
                if move != table[current_board][1]:
                    table = dict()
                    result = minimax_B(current_board.make_move(move), -math.inf, math.inf, table)
            else:
                move = table[current_board][1]
                print("Computer chooses move: %d" % (move))
            current_board = current_board.make_move(move)
        print("Game over!")
        print(current_board.to_2d_string())
        if current_board.get_game_state() != GameState.TIE:
            winner = "MAX" if current_board.get_game_state() == GameState.MAX_WIN else "MIN"
            print("The winner is %s (%s)" % (winner, players[winner]))
        else:
            print("Draw!")
        again = input("Play again? (y/n): ")
        if again == "n": break
        # Regenerate the table
        table = dict()
        minimax_B(board, -math.inf, math.inf, table)

# Play part A
def play_A(debugging, r, c, inarow):
    table = dict()
    board = Board(r, c, inarow)
    start = time()
    result = minimax_A(board, table)
    end = time()
    if debugging:
        for key in table.keys():
            print(key.to_2d_string())
            print(table[key])
    print("Search completed in %.3f seconds." % (end - start))
    print("Transposition table has %d states." % (len(table.keys())))
    if result[0] == 0:
        print("The game results in a draw with perfect play")
    elif result[0] > 0:
        print("First player has a guaranteed win with perfect play.")
    else:
        print("Second player has a guaranteed win with perfect play.")
    first_player = int(input("Who plays first? 1=human, 2=computer: "))
    players = dict()
    if first_player == 1:
        players["MAX"] = "human"
        players["MIN"] = "computer"
    else:
        players["MAX"] = "computer"
        players["MIN"] = "human"
    while True:
        current_board = Board(r, c, inarow)
        while current_board.get_game_state() == GameState.IN_PROGRESS:
            print(current_board.to_2d_string())
            print("Minimax value for this state: %d, optimal move: %d" % (table[current_board][0], table[current_board][1]))
            next_player = "MAX" if current_board.get_player_to_move_next() == Player.MAX else "MIN"
            print("It is %s's turn!" % (next_player))
            if players[next_player] == "human":
                move = int(input("Enter move: "))
            else:
                move = table[current_board][1]
                print("Computer chooses move: %d" % (move))
            current_board = current_board.make_move(move)
        print("Game over!")
        print(current_board.to_2d_string())
        if current_board.get_game_state() != GameState.TIE:
            winner = "MAX" if current_board.get_game_state() == GameState.MAX_WIN else "MIN"
            print("The winner is %s (%s)" % (winner, players[winner]))
        else:
            print("Draw!")
        again = input("Play again? (y/n): ")
        if again == "n": break

def main():
    algorithm = input("Run part A, B, or C? ")
    debugging = input("Include debugging info? (y/n) ") == "y"
    r = int(input("Enter rows: "))
    c = int(input("Enter columns: "))
    inarow = int(input("Enter number in a row to win: "))
    if algorithm.casefold() == "a":
        play_A(debugging, r, c, inarow)
    elif algorithm.casefold() == "b":
        play_B(debugging, r, c, inarow)
    else:
        play_C(debugging, r, c, inarow)
    
main()
    