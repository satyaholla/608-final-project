import sqlite3


example_db = '/var/jail/home/shruthi/final/random.db'

def create_database():
    conn = sqlite3.connect(example_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS message_table (sender text,recipient text, message text);''') # run a CREATE TABLE command
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
        things = c.execute('''SELECT DISTINCT * FROM message_table WHERE recipient = ?;''',(recipient,)).fetchall()
        for row in things:
            message_list.append(row)
        conn.commit()
        conn.close()
        if not message_list: return 'No messages yet!'
        msg_from = message_list[-1][0]
        msg = message_list[-1][2]
        out = f"{msg_from} says: \n {msg}"
        return out
        #return message_list
    if request["method"] == "POST": #from sender
        #post into the database
        #{'method': 'POST', 'args': ['sender'], 'values': {'sender': ''}, 
        # 'content-type': 'application/x-www-form-urlencoded',
        #'is_json': False, 'data': b'', 
        # 'form': {'sender': 'kyle', 'recipient': 'jeremy', 'message': 'hi jeremy this is kyle'}}
        message_list = []
        sender = request["form"]["sender"]
        recipient = request["form"]["recipient"]
        message = request["form"]["message"]
        
        conn = sqlite3.connect(example_db)
        c = conn.cursor()
        c.execute('''INSERT into message_table VALUES (?,?,?);''',(sender, recipient, message))
        things = c.execute('''SELECT * FROM message_table;''').fetchall()
        for row in things:
            message_list.append(row)
        conn.commit()
        conn.close()

        return message_list[-1]