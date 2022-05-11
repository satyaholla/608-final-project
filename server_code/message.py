import sqlite3
import datetime


db = '/var/jail/home/team33/final/messages.db'
token_db = '/var/jail/home/team33/final/logininfo.db'

def create_database():
    conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS message_table (sender text, recipient text, message text, read int, timing timestamp);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database

def request_handler(request):
    create_database()
    if request["method"] == "GET": #from receiver
        try:
            recipient = request["values"]["recipient"]
            token = int(request["values"]["token"])
        except: return 'Key Error'
        # if not verify_token(recipient, token): return '#'
        conn = sqlite3.connect(db)
        c = conn.cursor()
        latest_message = c.execute('''SELECT sender, message FROM message_table WHERE recipient=? AND read = 0 ORDER BY timing DESC;''', (recipient,)).fetchone()
        c.execute('''UPDATE message_table SET read = 1 WHERE read = 0;''')
        conn.commit()
        conn.close()
        return f"{latest_message[0],latest_message[1]}" if latest_message is not None else '^'

    if request["method"] == "POST": #from sender
        #post into the database
        #{'method': 'POST', 'args': ['sender'], 'values': {'sender': ''}, 
        # 'content-type': 'application/x-www-form-urlencoded',
        #'is_json': False, 'data': b'', 
        # 'form': {'sender': 'kyle', 'recipient': 'jeremy', 'message': 'hi jeremy this is kyle'}}
        try:
            sender = request["form"]["sender"]
            recipient = request["form"]["recipient"]
            message = request["form"]["message"]
            token = int(request["form"]["token"])
        except: return 'Key error'

        # if not verify_token(sender, token): return '#'
        conn = sqlite3.connect(db)
        c = conn.cursor()
        c.execute('''INSERT into message_table VALUES (?,?,?,?,?);''',(sender, recipient, message,0, datetime.datetime.now()))
        conn.commit()
        conn.close()

        return 'Successfully posted'

def verify_token(user, token):
    conn = sqlite3.connect(token_db)
    c = conn.cursor()
    online_token = int(c.execute('''SELECT token from tokens WHERE user=?;''', (user,)).fetchall()[-1][0])
    conn.commit()
    conn.close()
    return token == online_token

def return_all_messages():
    conn = sqlite3.connect(db)
    c = conn.cursor()
    res = (c.execute('''SELECT * from message_table;''').fetchall())
    conn.commit()
    conn.close()
    return res