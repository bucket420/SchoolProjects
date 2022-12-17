import math

# Return a dictionary that maps a location to its coordinates
def get_locations(file):
    file = open(file)
    loc_dict = {}
    for line in file:
        line = line.strip()
        line = line.split('|')
        # print(line)
        if line[0] == 'location':
            loc_dict[line[1]] = (line[2], line[3])
    file.close()
    return loc_dict            

# Return a dictionary that maps a location to a list of roads that intersect with it
def get_roads(file):
    file = open(file)
    loc_to_roads = {}
    for line in file:
        line = line.strip()
        line = line.split('|')
        if line[0] == 'road':
            if line[1] not in loc_to_roads.keys():
                loc_to_roads[line[1]] = [(line[2], line[3], line[4])]
            else:
                loc_to_roads[line[1]].append((line[2], line[3], line[4]))
            if line[2] not in loc_to_roads.keys():
                loc_to_roads[line[2]] = [(line[1], line[3], line[4])]
            else:
                loc_to_roads[line[2]].append((line[1], line[3], line[4]))
    file.close()
    return loc_to_roads  

# Calculate distance between two points
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
    return arc

# Calculate time to pass the road
def time_to_pass(loc1, loc2, loc_dict, speed):
    distance = 3960.0 * distance_on_unit_sphere(float(loc_dict[loc1][0]), float(loc_dict[loc1][1]), float(loc_dict[loc2][0]), float(loc_dict[loc2][1]))
    return distance * 3600.0 / speed 

def main():
    file = input('Enter a filename: ')
    locations = get_locations(file)
    loc_to_roads = get_roads(file)
    location = input('Enter a location ID or 0 to quit: ')
    while location != '0':
        print('Location %s has roads leading to:\n' %(location))
        for road in loc_to_roads[location]:
            print('Location %s, %d mph, %s, %.15f seconds' %(road[0], int(road[1]), road[2], time_to_pass(location, road[0], locations, float(road[1]))))
        print('\n')
        location = input('Enter a location ID or 0 to quit: ')
        
main()
