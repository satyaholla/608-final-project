import sqlite3
import random
import math
ht_db = '/var/jail/home/team33/final/logininfo.db' 

def request_handler(request):
    with sqlite3.connect(ht_db) as c:
        c.execute("""CREATE TABLE IF NOT EXISTS login (user, password);""")
        if request["method"]=="POST":
            user=request['form']['user']
            password=request['form']['password']
            function=request['form']['function']                
            correct_password_list=c.execute('''SELECT password FROM login WHERE user = ?;''',(user,)).fetchall()
            if function=="signup":
                if len(correct_password_list)==0:
                    c.execute('''INSERT into login VALUES (?,?);''',(user,password))
                    return "Successful Signup"
                else:
                    return "Username already exists, pick a different one"
            elif function=="login":
                if(len(correct_password_list)==0):
                    return "No account with this username"
                else:
                    c.execute("""CREATE TABLE IF NOT EXISTS tokens (user, token);""")
                    correct_password=str(correct_password_list[0])[2:-3]
                    if(password==correct_password):
                        token=math.floor(random.random()*100000)
                        c.execute('''INSERT into tokens VALUES (?,?);''',(user,token))
                        return token
                    else:
                        return "Wrong Password"
        elif request["method"]=="GET":
            return "No Get Requests"