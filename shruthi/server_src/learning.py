#create the database
import sqlite3
import random
import string


example_db = '/var/jail/home/shruthi/gesture/random.db'

def create_database():
    conn = sqlite3.connect(example_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS gestures_table (number int,x_vals text, y_vals text);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database


gest_list = []

def request_handler(request):
    if request["method"] == "GET":
        conn = sqlite3.connect(example_db)
        c = conn.cursor()
        things = c.execute('''SELECT * FROM gestures_table;''').fetchall() #WHERE number=1
        for row in things:
            gest_list.append(row)
        conn.commit()
        conn.close()
        return gest_list
    if request["method"] == "POST":
        create_database()  #call the function!
       
        number = int(request["form"]["number"])
        x_vals = request["form"]["x_vals"]
        y_vals = request["form"]["y_vals"]

        #send these values to the database
        conn = sqlite3.connect(example_db)
        c = conn.cursor()
        c.execute('''INSERT into gestures_table VALUES (%d, %s, %s);'''%(number, x_vals, y_vals))
        conn.commit()
        conn.close()

        return (number, x_vals, y_vals)
        #have the esp retrieve values from the database
        