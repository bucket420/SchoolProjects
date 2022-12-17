from board import Board

def main():
	board = Board(6, 7, 4) # standard connect 4 size
	print("Here is the board:")
	print(board.to_2d_string())

	# MAX(X) makes a move
	board = board.make_move(3)
	print("Here is the updated board:")
	print(board.to_2d_string())

	# MIN(O) makes a move
	board = board.make_move(2)
	print("Here is the updated board:")
	print(board.to_2d_string())

	# Check	the game progress:
	print("State of the game:", board.get_game_state())

	# Demo with a smaller board:
	board = Board(3, 3, 2) # a rather silly game
	print("Here is the board:")
	print(board.to_2d_string())

	# MAX(X) makes a move
	board = board.make_move(2)
	print("Here is the updated board:")
	print(board.to_2d_string())

	# MIN(O) makes a move
	board = board.make_move(1)
	print("Here is the updated board:")
	print(board.to_2d_string())

	# MAX(X) makes a move
	board = board.make_move(2)
	print("Here is the updated board:")
	print(board.to_2d_string())

	# Check	the game progress:
	print("State of the game:", board.get_game_state())

main()
