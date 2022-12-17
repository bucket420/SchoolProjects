# Return a 1D array of all positions on the grid. Each location is represented by a 2-tuple (row, column)
def all_locations(m, n):
    locations = []
    for i in range(m):
        for j in range(n):
            locations.append((i, j))
    return locations

# Return the initial probability distribution of the monkey's location      
def initial_distribution(m, n):
    locations = []
    for i in range(m * n):
        locations.append(1 / m / n)
    return locations

# Return a list of locations that are one step away from current location         
def one_step_away(location, m, n):
    result = []
    if location[0] > 0: 
        result.append((location[0] - 1, location[1]))
    if location[0] < m - 1: 
        result.append((location[0] + 1, location[1]))
    if location[1] > 0: 
        result.append((location[0], location[1] - 1))
    if location[1] < n - 1: 
        result.append((location[0], location[1] + 1))
    return result
    
# Return a list of locations that are two steps away from current location         
def two_steps_away(location, m, n):
    result = []
    for one_step in one_step_away(location, m, n):
        for two_steps in one_step_away(one_step, m, n):
            if two_steps not in result and two_steps != location:
                result.append(two_steps)
    return result

# Calculate P(m | c). m can be m1 or m2.
def cal_m_given_c(sensor, location):
    if sensor[0] != location[0] and sensor[1] != location[1]:
        return 0.05
    else:
        return 0.9 - 0.1 * (abs(sensor[0] - location[0]) + abs(sensor[1] - location[1]))

# Calculate P(c | l)
def cal_c_given_l(current_location, last_location, m, n):
    possible_locations = one_step_away(last_location, m, n)
    if current_location not in possible_locations:
        return 0
    else:
        return 1 / len(possible_locations)
  
# Calculate P(s | c)  
def cal_s_given_c(sensed_location, current_location, m, n):
    if sensed_location == current_location:
        return 0.6
    elif sensed_location in one_step_away(current_location, m, n):
        return 0.3 / len(one_step_away(current_location, m, n))
    elif sensed_location in two_steps_away(current_location, m, n):
        return 0.1 / len(two_steps_away(current_location, m, n))
    else:
        return 0

# Return a normalized probability distribution
def normalize(distribution):
    normalized_distribution = []
    alpha = 1 / sum(distribution)
    for probability in distribution:
        normalized_distribution.append(probability * alpha)
    return normalized_distribution
    
# Return the probability distribution of the monkey's current location P(c | m1, m2, s)
def c_given_m1_m2_s_distribution(l_distribution, m1, m2, s, m, n):
    locations = all_locations(m, n)
    result = []
    for location in locations:
        probability = 0
        m1_given_c = cal_m_given_c((0, 0), location) if m1 == 1 else 1 - cal_m_given_c((0, 0), location)
        m2_given_c = cal_m_given_c((m-1, n-1), location) if m2 == 1 else 1 - cal_m_given_c((m-1, n-1), location)
        s_given_c = cal_s_given_c(s, location, m, n)
        for i in range(len(l_distribution)):
            l = l_distribution[i]
            c_given_l = cal_c_given_l(location, locations[i], m, n)
            probability += l * c_given_l * m1_given_c * m2_given_c * s_given_c
        result.append(probability)
    return normalize(result)
    
 
def main():
    # Read the file
    filename = "monkey4.txt"
    file = open(filename)
    size = file.readline().rstrip().split(" ")
    lines = file.readlines()
    file.close()
    m = int(size[0])
    n = int(size[1])
    l_distribution = initial_distribution(m, n)
    
    # Initial distribution
    print("Initial distribution of monkey's last location:")
    for j in range(m*n):
        print("%.8f" % (l_distribution[j]), end = "\n" if j % n == n-1 else " ")
    print("")
    
    # Print the distribution for each observation
    for i in range(len(lines)):
        evidence = lines[i].rstrip().split(" ")
        m1 = int(evidence[0])
        m2 = int(evidence[1])
        s = (int(evidence[2]), int(evidence[3]))
        
        print("Observation: Motion1: %s, Motion2: %s, Sound location: (%d, %d)" % (m1 == 1, m2 == 1, s[0], s[1]))
        print("Monkey's predicted current location at time step: %d" % (i))
        
        distribution = c_given_m1_m2_s_distribution(l_distribution, m1, m2, s, m, n)
        l_distribution = distribution   # Update the probability distribution of 
        for j in range(m*n):
            print("%.8f" % (distribution[j]), end = "\n" if j % n == n-1 else " ")
        print("")
        
main()