import sqlite3
import datetime


example_db = '/var/jail/home/team33/final/messages.db'

def create_database():
    conn = sqlite3.connect(example_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS message_table (sender text, recipient text, message text, read int, timing timestamp);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database

def request_handler(request):
    create_database()
    if request["method"] == "GET": #from receiver
        #{'method': 'GET', 'args': ['recipient'], 'values': {'recipient': 'kyle'}}
        message_list = []
        recipient = request["values"]["recipient"]
        conn = sqlite3.connect(example_db)
        c = conn.cursor()
        things = c.execute('''SELECT sender, message FROM message_table WHERE recipient = ? AND read = 0 ORDER BY timing DESC;''', (recipient,)).fetchone()
        c.execute('''UPDATE message_table SET read = 1 WHERE read = 0;''')
        conn.commit()
        conn.close()
        return f"{[f'{thing[0]},{thing[1]}' for thing in things]}"

    if request["method"] == "POST": #from sender
        #post into the database
        #{'method': 'POST', 'args': ['sender'], 'values': {'sender': ''}, 
        # 'content-type': 'application/x-www-form-urlencoded',
        #'is_json': False, 'data': b'', 
        # 'form': {'sender': 'kyle', 'recipient': 'jeremy', 'message': 'hi jeremy this is kyle'}}
        sender = request["form"]["sender"]
        recipient = request["form"]["recipient"]
        message = request["form"]["message"]
        
        conn = sqlite3.connect(example_db)
        c = conn.cursor()
        c.execute('''INSERT into message_table VALUES (?,?,?,?,?);''',(sender, recipient, message,0, datetime.datetime.now()))
        conn.commit()
        conn.close()

        return 'Successfully posted'