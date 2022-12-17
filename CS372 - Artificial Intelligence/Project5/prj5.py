import random

# Return the new state after taking an action
def move(state, action):
    new_state = ""
    new_state += "B" if state[0] == "A" else "A"
    for i in range(1, len(state)):
        if i == int(action[0]) + 1:
            new_state += str(int(state[int(action[0]) + 1]) - int(action[1]))
        else:
            new_state += state[i]
    return new_state

# Return a list of possible actions
def actions(state):
    moves = []
    for i in range(1, len(state)):
        if int(state[i]) == 0:
            continue
        for j in range(1, int(state[i]) + 1):
            moves.append(str(i-1) + str(j))
    return moves

# Generate the initial table of Q values
def q_table(state):
    table = dict()
    q_table_helper(state, table)
    return table

# Helper for the above function
def q_table_helper(state, table):
    if state[1:] == "000":
        return
    for action in actions(state):
        if state + action not in table.keys():
            table[state + action] = 0
            q_table_helper(move(state, action), table)

# Return the min/max Q value from a state
def extremum(state, q_table):
    if state[1:] == "000":
        return 0
    elif state[0] == "A":
        return max([q_table[state + action] for action in actions(state)])
    else:
        return min([q_table[state + action] for action in actions(state)])

# Return the optimal policy of a state  
def pi(state, q_table):
    reward = extremum(state, q_table)
    for action in actions(state):
        if q_table[state + action] == reward:
            return action
        
# Return the reward of a state
def reward(state):
    if state[1:] != "000":
        return 0
    elif state[0] == "A":
        return 1000
    else:
        return -1000

# Play one full game
def simulate(state, table):
    while state[1:] != "000":
        action = random.choice(actions(state))
        next_state = move(state, action)
        table[state + action] = reward(next_state) + 0.9 * extremum(next_state, table)
        state = next_state
     
# Let user play against computer   
def play(state, table):
    players = dict()
    players["A"] = "user" if input("Who moves first, (1) User or (2) Computer? ") == "1" else "computer"
    players["B"] = "user" if players["A"] == "computer" else "computer"
    print()
    while state[1:] != "000":
        current_player = state[0]
        action = ""
        print("Player %s (%s)'s turn; board is (%s, %s, %s)." % (current_player, players[current_player], state[1], state[2], state[3]))
        if players[current_player] == "computer":
            action = pi(state, table)
            print("Computer chooses pile %s and removes %s." % (action[0], action[1]))
        else:
            action = "".join([input("What pile? "), input("How many? ")])
        state = move(state, action)
        print()
    print("Game Over.\nWinner is %s (%s)." % (state[0], players[state[0]]))
        

def main():
    # Generate the table of Q values
    initial_state = "A" + "".join([input("Number in pile %d? " % (i)) for i in range(3)])
    print()
    table = q_table(initial_state)
    games = int(input("Number of games to simulate? "))
    for i in range(games):
        simulate(initial_state, table)
    print()
    
    # Print out the content of the table
    print("Final Q-values:\n")
    keys = list(table.keys())
    keys.sort()
    for key in keys:
        print("Q[%s, %s] = %.1f" % (key[0:4], key[4:], table[key]))
    print()
    
    # Play the game
    while (True):
        play(initial_state, table)
        print()
        if input("Play again? (1) Yes (2) No: ") == "2":
            break
  
main()
    