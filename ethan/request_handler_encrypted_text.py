import sqlite3
ht_db='/var/jail/home/team33/final/logininfo.db'

def request_handler(request):
    with sqlite3.connect(ht_db) as c:
        try:
            user=request['form']['sender']
            token=(request['form']['token'])
        except:
            user=request['values']['receiver']
            token=(request['values']['token'])
        token_list=c.execute('''SELECT token FROM tokens WHERE user = ?;''',(user,)).fetchall()
        try:
            correct_token=(str(token_list[-1])[1:-2])
        except:
            return "Must Signup/Login First"
        if(token!=correct_token):
            return "Wrong Token"
        c.execute("""CREATE TABLE IF NOT EXISTS messages (sender, receiver, message);""")
        if request["method"]=="POST":
            sender=request['form']['sender']
            receiver=request['form']['receiver']
            message=request['form']['message']
            c.execute('''INSERT into messages VALUES (?,?,?);''',(sender,receiver,message))
            return "Message posted successfully"
        elif request["method"]=="GET":
            receiver=request['values']['receiver']
            message_list=c.execute('''SELECT * FROM messages WHERE receiver = ?;''',(receiver,)).fetchall()
            try: 
                message_info=str(message_list[-1]).split(',')
                return "From "+message_info[0][2:-1]+": "+message_info[2][2:-2]
            except:
                return "No messages to see"