import random
from math import prod
import sqlite3
import datetime
db = 'key_exchange.db'

SG_PRIME = 1048433
SAFE_PRIME = SG_PRIME*2 + 1
FACTORIZATION = {2: 1, SG_PRIME: 1}

def request_handler(request):
    initialize_tables()
    if request['method'] == 'GET':
        try:
            values = request['values']
            get_type = values['get_type']
            other_user, this_user = values['other_user'], values['this_user']
        except KeyError:
            return 'Key error'

        if get_type == 'gen_exchange':
            gen_res = get_gen_exchange(getter=this_user, poster=other_user) # try adding a delay to fix concurrency issues
            if gen_res is not None:
                p, g = gen_res
                return f'{p},{g}'
            else:
                p, g = SAFE_PRIME, find_generator(SAFE_PRIME, FACTORIZATION)
                post_gen_exchange(poster=this_user, getter=other_user, p=p, g=g)
                return f'{p},{g}'

        elif get_type == 'exp_exchange':
            return f'{get_exp_exchange(getter=this_user, poster=other_user)}'

        else: return 'Not a valid type of get request'

    elif request['method'] == 'POST':
        try:
            values = request['form']
            post_type = values['post_type']
        except KeyError:
            return 'Key Error'
        
        if post_type == 'exp_exchange':
            other_user, this_user = values['other_user'], values['this_user']
            exponent = int(values['exponent'])
            post_exp_exchange(poster=this_user, getter=other_user, exponent=exponent)
            return f'Received {exponent}'

        else:
            return 'Not a valid type of post request'

    else:
        return 'Request method invalid.'

def find_generator(prime, factorization):
    '''
    Finds a generator of Z_{prime}^*, given factorization, a dictionary containing key: val pairs
    such that prime - 1 == prod(key**val for key, val in factorization.items())
    '''
    assert prod(key**val for key, val in factorization.items()) == prime - 1
    while True:
        attempt = random.randint(1, prime - 1)
        is_generator = all(find_power(attempt, (prime - 1)//factor, prime) != 1 for factor in factorization)
        if is_generator: return attempt

def find_power(g, x, p):
    '''
    Efficiently computes g**x (mod p) using repeated squaring
    '''
    binary = []
    while x != 0:
        binary.append(x % 2)
        x //= 2
    res = 1
    for bit in reversed(binary):
        res **= 2
        if bit == 1: res *= g
        res %= p
    return res

def connect_db(func):
    def inner(*args, **kwargs):
    
        func(*args, **kwargs)
    
        return 2
    return inner

def initialize_tables():
    conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS gen_table (poster text, getter text, p int, g int, timing timestamp);''') # run a CREATE TABLE command
    c.execute('''CREATE TABLE IF NOT EXISTS exp_table (poster text, getter text, exp int, timing timestamp);''') # run a CREATE TABLE command
    c.execute('''DELETE FROM gen_table WHERE timing < ?;''', (datetime.datetime.now() - datetime.timedelta(minutes = 5),))
    c.execute('''DELETE FROM exp_table WHERE timing < ?;''', (datetime.datetime.now() - datetime.timedelta(minutes = 5),))
    conn.commit()
    conn.close()

def get_gen_exchange(poster, getter):
    conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    gen_res = c.execute('''SELECT * FROM gen_table WHERE poster=? AND getter=? ORDER BY timing DESC;''', (poster, getter)).fetchone()
    conn.commit()
    conn.close()
    return gen_res if gen_res is None else gen_res[2:4]

def post_gen_exchange(poster, getter, p, g):
    conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''INSERT into gen_table VALUES (?,?,?,?,?);''',(poster, getter, p, g, datetime.datetime.now()))
    conn.commit()
    conn.close()

def get_exp_exchange(poster, getter):
    conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    gen_res = c.execute('''SELECT * FROM exp_table WHERE poster=? AND getter=? ORDER BY timing DESC;''', (poster, getter)).fetchone()
    conn.commit()
    conn.close()
    return 'x' if gen_res is None else gen_res[2]

def post_exp_exchange(poster, getter, exponent):
    conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''INSERT into exp_table VALUES (?,?,?,?);''',(poster, getter, exponent, datetime.datetime.now()))
    conn.commit()
    conn.close()

def request_handler_test(method, **kwargs):
    request_dict = {'method': method}
    if method == 'GET':
        request_dict['values'] = kwargs
    elif method == 'POST':
        request_dict['form'] = kwargs
    else: raise Exception()
    return request_handler(request_dict)


if __name__ == '__main__':
    g = set()
    for i in range(1000):
        g.add(find_generator(13, {3:1, 2:2}))
    print(g)
