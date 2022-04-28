import sqlite3
import datetime
from math import radians, cos, sin, asin, sqrt

locations = {
    "Student Center": [(-71.095863,42.357307),(-71.097730,42.359075),(-71.095102,42.360295),(-71.093900,42.359340),(-71.093289,42.358306)],
    "Dorm Row": [(-71.093117,42.358147),(-71.092559,42.357069),(-71.102987,42.353866),(-71.106292,42.353517)],
    "Simmons/Briggs": [(-71.097859,42.359035),(-71.095928,42.357243),(-71.106356,42.353580),(-71.108159,42.354468)],
    "Boston FSILG (West)": [(-71.124664,42.353342),(-71.125737,42.344906),(-71.092478,42.348014),(-71.092607,42.350266)],
    "Boston FSILG (East)": [(-71.092409,42.351392),(-71.090842,42.343589),(-71.080478,42.350900),(-71.081766,42.353771)],
    "Stata/North Court": [(-71.091636,42.361802),(-71.090950,42.360811),(-71.088353,42.361112),(-71.088267,42.362476),(-71.089769,42.362618)],
    "East Campus": [(-71.089426,42.358306),(-71.090885,42.360716),(-71.088310,42.361017),(-71.087130,42.359162)],
    "Vassar Academic Buildings": [(-71.094973,42.360359),(-71.091776,42.361770),(-71.090928,42.360636),(-71.094040,42.359574)],
    "Infinite Corridor/Killian": [(-71.093932,42.359542),(-71.092259,42.357180),(-71.089619,42.358274),(-71.090928,42.360541)],
    "Kendall Square": [(-71.088117,42.364188),(-71.088225,42.361112),(-71.082774,42.362032)],
    "Sloan/Media Lab": [(-71.088203,42.361017),(-71.087044,42.359178),(-71.080071,42.361619),(-71.082796,42.361905)],
    "North Campus": [(-71.11022,42.355325),(-71.101280,42.363934),(-71.089950,42.362666),(-71.108361,42.354484)],
    "Technology Square": [(-71.093610,42.363157),(-71.092130,42.365837),(-71.088182,42.364188),(-71.088267,42.362650)]
}

def bounding_box(point_coord,box):
    x_vals = [coord[0] for coord in box]
    y_vals = [coord[1] for coord in box]
    min_x, max_x = min(x_vals), max(x_vals)
    min_y, max_y = min(y_vals), max(y_vals)
    return (min_x < point_coord[0] < max_x) and (min_y < point_coord[1] < max_y)

def translate(point, origin):
    return (point[0] - origin[0], point[1] - origin[1])

def within_area(point_coord,poly):
    crossings = 0
    centered_poly = [translate(vertex, point_coord) for vertex in poly]
    
    for i in range(len(centered_poly)):
        sx, sy = centered_poly[i]
        ex, ey = centered_poly[(i + 1)%len(centered_poly)]
        if sign(sy) * sign(ey) == -1:
            if sx > 0 and ex > 0:
                crossings += 1
            elif sx < 0 and ex < 0:
                continue
            else:
                x_intersect = (sx * ey - sy * ex)/(ey - sy)
                if x_intersect > 0:
                    crossings += 1
    
    return (crossings % 2 == 1)


def sign(x):
    if x > 0:
        return 1
    elif x == 0:
        return 0
    else:
        return -1

def get_area(point_coord,locations):
    for name, box in locations.items():
        if within_area(point_coord, box):
            return name
    return "Off Campus"

def haversine(lon1, lat1, lon2, lat2):
    """
    Calculate the great circle distance in kilometers between two points 
    on the earth (specified in decimal degrees)
    """
    # convert decimal degrees to radians 
    lon1, lat1, lon2, lat2 = map(radians, [lon1, lat1, lon2, lat2])

    # haversine formula 
    dlon = lon2 - lon1 
    dlat = lat2 - lat1 
    a = sin(dlat/2)**2 + cos(lat1) * cos(lat2) * sin(dlon/2)**2
    c = 2 * asin(sqrt(a)) 
    r = 6371 # Radius of earth in kilometers. Use 3956 for miles. Determines return value units.
    return c * r

def distance(entry1, entry2):
    _, lat1, lon1, _, _ = entry1
    _, lat2, lon2, _, _ = entry2
    return haversine(lon1, lat1, lon2, lat2)

def close_enough(entry, last_entry):
    return distance(entry, last_entry) <= 0.02

visits_db = '/var/jail/home/mehrab/lab5b/time_example2.db'

def request_handler(request):
    if request["method"] == "GET":
        lat = 0
        lon = 0
        # try:
            # lat = float(request['values']['lat'])
            # lon = float(request['values']['lon'])
        # except Exception as e:
            # return e here or use your own custom return message for error catch
            #be careful just copy-pasting the try except as it stands since it will catch *all* Exceptions not just ones related to number conversion.
            # return "Error: lat, lon are missing or not numbers"

        # return get_area([lon,lat],locations)
        # return str(f'{lat}, {lon}')

        # connect to that database (will create if it doesn't already exist)
        conn = sqlite3.connect(visits_db)  

        # move cursor into database (allows us to execute commands)
        c = conn.cursor()  
        outs = ""

        # run a CREATE TABLE command
        c.execute('''CREATE TABLE IF NOT EXISTS dated_table (user text, lat real, lon real, area text, timing timestamp);''') 

        one_day_ago = datetime.datetime.now() - datetime.timedelta(hours = 24)

        recent_people = c.execute(
            '''SELECT * FROM dated_table WHERE timing > ? ORDER BY timing DESC''', 
            (one_day_ago,)
        ).fetchall()
        # return recent_people
        if len(recent_people) == 0:
            return 'None found'
        most_recent_entry = recent_people[0]
        near_recent_people = [ p for p in recent_people if close_enough(p, most_recent_entry) ]
        # return recent_people[0] + ' said "I was here!!" at ' + recent_people[-1] + ' while at ' + recent_people[-2]

        # outs = "Things:\n"
        # outs = area + "\n"
        # for person in recent_people:
        #     outs += str(person[0]) + "\n"

        outs = ""
        for entry in near_recent_people:
            user, lat, lon, area, timing = entry
            outs += user + ' said "I was here!!" about ' + str(round(1000*distance(entry, most_recent_entry), 2)) + 'm of you at ' + timing +' at (' + str(lat) + ',' + str(lon) + ')\r'
            # outs += str(user) + ' said "I was here!!" within ' + str(distance(entry, most_recent_entry)) + 'km of you at ' timing + ' at (' + str(lat) + ',' + str(lon) + ')\n'

        # commit commands
        conn.commit() 

        # close connection to database
        conn.close() 

        return outs
    else:
        # return 'POST not supported. You need to change that.'
        user = request['form']['user']
        lat = float(request['form']['lat'])
        lon = float(request['form']['lon'])
        area = get_area([lon, lat], locations)

        # connect to that database (will create if it doesn't already exist)
        conn = sqlite3.connect(visits_db)  

        # move cursor into database (allows us to execute commands)
        c = conn.cursor()  
        outs = ""

        # run a CREATE TABLE command
        c.execute('''CREATE TABLE IF NOT EXISTS dated_table (user text, lat real, lon real, area text, timing timestamp);''') 

        # create time for fifteen minutes ago!
        thirty_seconds_ago = datetime.datetime.now() - datetime.timedelta(seconds = 30)

        c.execute('''INSERT into dated_table VALUES (?,?,?,?,?);''', (user, lat, lon, area, datetime.datetime.now()))
        recent_people = c.execute(
            '''SELECT DISTINCT user FROM dated_table 
                    WHERE timing > ? AND area = ? 
                    ORDER BY timing ASC;''',
            (thirty_seconds_ago, area)
        ).fetchall()

        # outs = "Things:\n"
        outs = area + "\n"
        for person in recent_people:
            outs += str(person[0]) + "\n"

        # commit commands
        conn.commit() 

        # close connection to database
        conn.close() 

        return outs
