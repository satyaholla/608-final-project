import sqlite3


example_db = '/var/jail/home/shruthi/final/random.db'

def create_database():
    conn = sqlite3.connect(example_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS message_table (sender text, recipient text, message text, read int);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database

create_database()


def request_handler(request):
    if request["method"] == "GET": #from receiver
        #{'method': 'GET', 'args': ['recipient'], 'values': {'recipient': 'kyle'}}
        message_list = []
        recipient = request["values"]["recipient"]
        conn = sqlite3.connect(example_db)
        c = conn.cursor()
        things = c.execute('''SELECT DISTINCT * FROM message_table WHERE recipient = ? AND read = ?;''',(recipient, 0)).fetchall()
        conn.commit()
        conn.close()
        return f"{[f'{thing[0]} says: {thing[2]}' for thing in things]}"

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
        c.execute('''INSERT into message_table VALUES (?,?,?);''',(sender, recipient, message))
        conn.commit()
        conn.close()

        return 'Successfully posted'