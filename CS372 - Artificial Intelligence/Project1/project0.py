import math

# probably could have written Location & Road using data classes.

class Location(object):
    """
    A Location object represents an intersection (vertex) in the road network.
    """
    def __init__(self, locId, latitude, longitude):
        self.locId = locId
        self.longitude = longitude
        self.latitude = latitude

    def __str__(self):
        return f'Location: {self.locId}, long={self.longitude}, lat={self.latitude}'

    def __repr__(self):
        return f'Location({self.locId}, {self.longitude}, {self.latitude})'


class Road(object):
    """
    A Road object represents a connection between Locations (an edge) in the road network.
    """
    def __init__(self, startId, endId, speed, name):
        self.startId = startId
        self.endId = endId
        self.speed = speed
        self.name = name

    def __str__(self):
        return f'Road: start={self.startId}, end={self.endId}, {self.speed}, {self.name}'

    def __repr__(self):
        return f'Road({self.startId}, {self.endId}, {self.speed}, {self.name})'


class RoadNetwork(object):
    """
    A RoadNetwork holds a graph of Locations and Roads.
    """

    def __init__(self):
        self.locations = dict()
        self.roads = dict()

    def add_location(self, location: Location):
        """
        Add a new Location to the network.
        :param location:
        :return:
        """
        self.locations[location.locId] = location
        self.roads[location.locId] = []

    def add_road(self, road: Road):
        """
        Add a new Road to the network.  We assume its starting and ending locations
        already exist in the network.
        """
        self.roads[road.startId].append(road)

    def get_location_by_id(self, locId):
        """
        Get a Location by its identifier.
        """
        return self.locations[locId]

    def get_roads_connected_to(self, locId):
        """
        Get a list of all the roads connected to a specific location id.
        """
        return self.roads[locId]
        
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


def main():
    graph = RoadNetwork()
    filename = input('Enter a filename: ')

    with open(filename, 'r') as file:
        for line in file:
            line = line.rstrip()
            pieces = line.split('|')

            if pieces[0] == 'location':
                _, locId, latitude, longitude = pieces
                locId = int(locId)
                longitude = float(longitude)
                latitude = float(latitude)
                loc = Location(locId, latitude, longitude)
                graph.add_location(loc)

            elif pieces[0] == 'road':
                _, startId, endId, speed, name = pieces
                startId = int(startId)
                endId = int(endId)
                speed = int(speed)
                road = Road(startId, endId, speed, name)
                graph.add_road(road)
                road_backwards = Road(endId, startId, speed, name)
                graph.add_road(road_backwards)

    while True:
        locId = int(input("\nEnter a location ID or 0 to quit: "))

        if locId == 0:
            break

        roads = graph.get_roads_connected_to(locId)
        loc1 = graph.get_location_by_id(locId)
        print(f"Location {locId} has roads leading to:")
        for road in roads:
            loc2 = graph.get_location_by_id(road.endId)
            dist = distance_on_unit_sphere(loc1.latitude, loc1.longitude, loc2.latitude, loc2.longitude) * 3960
            time_sec = dist / road.speed * 60 * 60
            print(f"    Location {road.endId}, {road.speed} mph, {road.name}, {time_sec} seconds")

 
main()
