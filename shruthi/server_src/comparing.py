import sqlite3
import numpy as np

example_db = '/var/jail/home/shruthi/gesture/random.db'
x_val = []
y_val = []

stored_x =[]
stored_y = []

"""
Functions to calculate the average. Returns an offset and normalized list that can then be dotted (numpy.dot) with the stored gestures' imu readings.
"""
def st(l1):
    avg1 = sum(l1)/len(l1)
    out = 0
    for i in range(len(l1)):
        out = out + (l1[i]-avg1)*(l1[i]-avg1)
    return float(out)**0.5

def offset_and_normalize(inp):
    avg = sum(inp)/len(inp)
    ls = []
    for i in range(len(inp)):
        ls.append((inp[i]-avg)/st(inp))
    return ls

def request_handler(request):
    if request["method"] == "POST":
        #{'method': 'POST', 'args': ['x_val', 'y_val'], 
        # 'values': {'x_val': '0.2 0.3 0.4 0.5', 'y_val': '0.001 0.002 0.0009 0.001'}, 
        # 'content-type': '', 'is_json': False, 'data': b'', 'form': {}}
        x_val = request["values"]["x_val"].split()
        for i in range(len(x_val)):
            x_val[i] = float(i)
        y_val = request["values"]["y_val"].split()
        for j in range(len(y_val)):
            y_val[j] = float(j)
        #get info from database and do the correlation then return it to the esp
        conn = sqlite3.connect(example_db)
        c = conn.cursor()
        x_things = c.execute('''SELECT x_vals FROM gestures_table ;''').fetchall()
        for row in x_things:
            stored_x.append(row)
        
        y_things = c.execute('''SELECT y_vals FROM gestures_table ;''').fetchall()
        for row in y_things:
            stored_y.append(row)

        #cycle through and calculate the correlation
        for gesture in stored_x:
            if np.dot(x_val,gesture) > max_corr:
                max_corr = np.dot(x_val,gesture)
                corr_gesture = gesture
        for gesture in stored_y:
            if np.dot(y_val,gesture) > max_corr:
                max_corr = np.dot(x_val,gesture)
                corr_gesture = gesture
        
        conn.commit()
        conn.close()

        return gesture