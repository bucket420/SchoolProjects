import math
from pqueue import PQueue

# Returns a dictionary that maps a location to its coordinates 
# { ID: (latitude, longitude) }
def get_locations(file):
    file = open(file)
    loc_dict = {}
    for line in file:
        line = line.strip()
        line = line.split('|')
        # print(line)
        if line[0] == 'location':
            loc_dict[line[1]] = (float(line[2]), float(line[3]))
    file.close()
    return loc_dict            

# Returns a dictionary that maps a location to a collection of locations that's connected to it 
# { ID1: {ID2 : (speed limit, road name)} }
def get_roads(file):
    file = open(file)
    loc_to_roads = {}
    for line in file:
        line = line.strip()
        line = line.split('|')
        if line[0] == 'road':
            if line[1] not in loc_to_roads.keys():
                loc_to_roads[line[1]] = { line[2] : (float(line[3]), line[4]) }
            else:
                loc_to_roads[line[1]][line[2]] = (float(line[3]), line[4])
            if line[2] not in loc_to_roads.keys():
                loc_to_roads[line[2]] = { line[1] : (float(line[3]), line[4]) }
            else:
                loc_to_roads[line[2]][line[1]] = (float(line[3]), line[4])
    file.close()
    return loc_to_roads  

# Calculates the distance between two locations
def distance_on_unit_sphere(lat1, long1, lat2, long2):

    # Convert latitude and longitude to
    # spherical coordinates in radians.
    degrees_to_radians = math.pi/180.0

    # phi = 90 - latitude
    phi1 = (90.0 - lat1)*degrees_to_radians
    phi2 = (90.0 - lat2)*degrees_to_radians

    # theta = longitude
    theta1 = long1*degrees_to_radians
    theta2 = long2*degrees_to_radians

    # Compute spherical distance from spherical coordinates.

    # For two locations in spherical coordinates
    # (1, theta, phi) and (1, theta', phi')
    # cosine( arc length ) =
    # sin phi sin phi' cos(theta-theta') + cos phi cos phi'
    # distance = rho * arc length

    cos = (math.sin(phi1)*math.sin(phi2)*math.cos(theta1 - theta2) +
    math.cos(phi1)*math.cos(phi2))
    arc = math.acos( cos )

    # Remember to multiply arc by the radius of the earth
    # in your favorite set of units to get length.
    return arc * 3960.0

# Node class
class Node(object):    
    def __init__(self, state, parent, cost):
        self.state = state          # Location ID
        self.parent = parent        # Parent node
        self.cost = cost            # Cost to reach from initial location

# Expands a node
def expand(node : Node, roads, locations):
    children = []
    for child in roads[node.state]:
        current_location = node.state
        next_location = child
        speed = roads[current_location][next_location][0]
        action_cost = distance_on_unit_sphere(locations[current_location][0], locations[current_location][1], locations[next_location][0], locations[next_location][1]) * 3600.0 / speed
        total_cost = node.cost + action_cost
        child_node = Node(child, node, total_cost)
        children.append(child_node)
    return children

# A* algorithm
def A_star(start, goal, locations, roads, debugging):
    start_node = Node(start, None, 0)
    node_count = 0          # Number of nodes visited
    frontier = PQueue()
    frontier.enqueue(start_node, 0)
    reached = { start_node.state : start_node}
    while not frontier.empty():
        node = frontier.dequeue()
        node_count += 1
        if debugging:
            print()
            heuristic = distance_on_unit_sphere(locations[node.state][0], locations[node.state][1], locations[goal][0], locations[goal][1]) * 3600.0 / 65.0
            fn = node.cost + heuristic
            if (node.parent is not None): 
                print("Visiting [State=%s, parent=%s, g=%.15f, h=%.15f, f=%.15f]"%(node.state, node.parent.state, node.cost, heuristic, fn))
            else: 
                print("Visiting [State=%s, parent=null, g=0.0, h=%.15f, f=%.15f]" %(node.state, heuristic, heuristic))
        if (node.state == goal):
            if debugging: print()
            return [node, node_count]
        for child in expand(node, roads, locations):
            heuristic = distance_on_unit_sphere(locations[child.state][0], locations[child.state][1], locations[goal][0], locations[goal][1]) * 3600.0 / 65.0
            fn = child.cost + heuristic
            if child.state not in reached.keys() or child.cost < reached[child.state].cost:
                reached[child.state] = child
                frontier.enqueue(child, fn)
                if debugging: 
                    print("     Adding [State=%s, parent=%s, g=%.15f, h=%.15f, f=%.15f] to frontier." %(child.state, child.parent.state, child.cost, heuristic, fn))
            elif debugging:
                print("     Skipping [State=%s, parent=%s, g=%.15f, h=%.15f, f=%.15f] (already on frontier with lower cost)." %(child.state, child.parent.state, child.cost, heuristic, fn))
    print("Failed")
    return

# Returns compass bearing
def get_bearing(lat1, long1, lat2, long2):
  lat1 *= math.pi/180
  lat2 *= math.pi/180
  long1 *= math.pi/180
  long2 *= math.pi/180
  y = math.sin(long2-long1) * math.cos(lat2)
  x = math.cos(lat1) * math.sin(lat2) - math.sin(lat1) * math.cos(lat2) * math.cos(long2-long1)
  angle = math.atan2(y, x)
  bearing = (angle * 180/math.pi + 360) % 360
  return bearing

# Map integers to directions
directions = { 0 : "north", 
               1 : "northeast",
               2 : "east",
               3 : "southheast",
               4 : "south",
               5 : "southwest",
               6 : "west",
               7 : "northwest" }
   
# Gets starting direction
def get_initial_direction(lat1, long1, lat2, long2):
    bearing = get_bearing(lat1, long1, lat2, long2)
    key = round(bearing / 45.0)
    if key == 8: key = 0
    return directions[key]

# Gets turning direction (lelft or right) 
def get_turning_direction(lat1, long1, lat2, long2, lat3, long3):
    bearing1 = get_bearing(lat1, long1, lat2, long2)    # direction on current road 
    bearing2 = get_bearing(lat2, long2, lat3, long3)    # direction on next road
    if 0 <= bearing1 - bearing2 < 180 or bearing2 - bearing1 >= 180:
        return "left"
    else: 
        return "right"
    
def main():
    # Reading the file
    file = "memphis-medium.txt"
    locations = get_locations(file)
    roads = get_roads(file)
    
    # Getting inputs
    start = input("Enter starting location ID: ")
    goal = input("Enter ending location ID: ")
    debugging = input("Do you want debugging information (y/n)? ") == "y"
    
    # Running A* algorithm 
    node, node_count = A_star(start, goal, locations, roads, debugging)
    
    # Printing results
    print("Total travel time in seconds: %.15f" %(node.cost))
    print("Number of nodes visited: %d\n" %(node_count))
    
    # Reversing the string of nodes
    path = [] 
    while (node.state != start):
        path.insert(0, node)
        node = node.parent
        
    # Printing the route found
    print("Route found is:")
    print("%s (%s)" %(start, "Starting Location"))
    for location in path:
        print("%s (%s)" %(location.state, roads[location.parent.state][location.state][1]))
    print()

    # GPS instructions begin
    print("GPS directions:")
    
    # The very first instruction
    current_location = start
    next_location = path[0].state
    print("Head %s on %s" %(get_initial_direction(locations[current_location][0], locations[current_location][1], locations[next_location][0], locations[next_location][1]), roads[current_location][next_location][1]))

    # Length of a road (not a road segment) and the time it takes to pass it
    road_length = 0
    drive_time = 0
    
    # Printing GPS instructions
    for i in range(len(path)-1):
        last_location = path[i].parent.state
        current_location = path[i].state
        next_location = path[i+1].state
        current_road_name = roads[last_location][current_location][1]
        next_road_name = roads[current_location][next_location][1]
        distance = distance_on_unit_sphere(locations[last_location][0], locations[last_location][1], locations[current_location][0], locations[current_location][1]) # distance traveled on last segment (from last_location to current_location)
        road_length += distance
        drive_time += distance * 3600.0 / roads[last_location][current_location][0]
        if current_road_name == next_road_name:
            continue
        else:
            print("     Drive for %.2f miles (%.2f seconds)" % (road_length, drive_time))
            road_length = 0
            drive_time = 0
            print("Turn %s onto %s" %(get_turning_direction(locations[last_location][0], locations[last_location][1], locations[current_location][0], locations[current_location][1], locations[next_location][0], locations[next_location][1]), roads[current_location][next_location][1]))
    
    # Adding the last segment and print the last instruction
    distance = distance_on_unit_sphere(locations[current_location][0], locations[current_location][1], locations[goal][0], locations[goal][1])
    road_length += distance
    drive_time += distance * 3600.0 / roads[current_location][goal][0]
    print("     Drive for %.2f miles (%.2f seconds)" % (road_length, drive_time))
    
    # Arrived!!!
    print("You have arrived!")

main()