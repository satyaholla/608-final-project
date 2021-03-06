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
            
        except KeyError:
            return 'Key error'

        if get_type == 'gen_exchange': # p,g if there was something in the database, ^ otherwise (so it has to post)
            try: other_user, this_user = values['other_user'], values['this_user']
            except KeyError: return 'Key error'
            gen_res = get_gen_exchange(getter=this_user, poster=other_user) # try adding a delay to fix concurrency issues
            if gen_res is not None:
                p, g = gen_res
                return f'{p},{g}'
            else:
                p, g = SAFE_PRIME, find_generator(SAFE_PRIME, FACTORIZATION)
                post_gen_exchange(poster=this_user, getter=other_user, p=p, g=g)
                return f'^'

        elif get_type == 'exp_exchange': # exponent if there's one in the database, ^ otherwise
            try: other_user, this_user = values['other_user'], values['this_user']
            except KeyError: return 'Key error'
            exp = get_exp_exchange(getter=this_user, poster=other_user)
            return '^' if exp is None else f'{exp[0]}'

        elif get_type == 'new_key_exchange': # 
            try: recipient = values['recipient']
            except KeyError: return 'Key error'
            other_user = get_new_key_exchange(recipient)
            return '^' if other_user is None else f'{other_user[0]}'

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
    c.execute('''CREATE TABLE IF NOT EXISTS gen_table (poster text, getter text, p int, g int, timing timestamp, new int);''') # run a CREATE TABLE command
    c.execute('''CREATE TABLE IF NOT EXISTS exp_table (poster text, getter text, exp int, timing timestamp);''') # run a CREATE TABLE command
    c.execute('''DELETE FROM gen_table WHERE timing < ?;''', (datetime.datetime.now() - datetime.timedelta(minutes = 5),))
    c.execute('''DELETE FROM exp_table WHERE timing < ?;''', (datetime.datetime.now() - datetime.timedelta(minutes = 5),))
    conn.commit()
    conn.close()

def get_gen_exchange(poster, getter):
    '''
    returns the most recent generator/prime between a poster and getter, or None if none exists
    '''
    conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    gen_res = c.execute('''SELECT p,g FROM gen_table WHERE (poster=? AND getter=?) OR (poster=? AND getter=?) ORDER BY timing DESC;''', (poster, getter, getter, poster)).fetchone()
    conn.commit()
    conn.close()
    return gen_res

def post_gen_exchange(poster, getter, p, g):
    '''
    inserts a generator/prime combo into the database
    '''
    conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''INSERT into gen_table VALUES (?,?,?,?,?,?);''',(poster, getter, p, g, datetime.datetime.now(),1))
    conn.commit()
    conn.close()

def get_new_key_exchange(recipient):
    '''
    returns the most recent user that tried a gen exchange with a user (that hasn't already been seen), or None if there are none
    '''
    conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    other_user = c.execute('''SELECT poster from gen_table WHERE getter=? AND new=1 ORDER BY timing DESC;''',(recipient,)).fetchone()
    c.execute('''UPDATE gen_table SET new=0 WHERE new=1 AND getter=?;''', (recipient,))
    conn.commit()
    conn.close()
    return other_user

def get_exp_exchange(poster, getter):
    '''
    returns the most recent exponent sent from poster to getter
    '''
    conn = sqlite3.connect(db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    exp_res = c.execute('''SELECT exp FROM exp_table WHERE poster=? AND getter=? ORDER BY timing DESC;''', (poster, getter)).fetchone()
    conn.commit()
    conn.close()
    return exp_res

def post_exp_exchange(poster, getter, exponent):
    '''
    posts an exponent to be sent from poster to getter to the database
    '''
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
    print(request_handler_test('GET', get_type='gen_exchange', other_user='ezahid', this_user='sgholla'))
    print(request_handler_test('GET', get_type='new_key_exchange', recipient='ezahid'))