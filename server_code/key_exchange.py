import random
from math import prod


active_users = dict()
SG_PRIME = 1048433
SAFE_PRIME = SG_PRIME*2 + 1
FACTORIZATION = {2: 1, SG_PRIME: 1}

def request_handler(request):
    if request['method'] == 'GET':
        try:
            values = request['values']
            user, this_user = values['user'], values['this_user']
        except KeyError:
            return 'Key error'

        if (this_user, user) in active_users:
            p, g = active_users[(this_user, user)]
            return f'{p},{g}'
        else:
            p, g = SAFE_PRIME, find_generator(SAFE_PRIME, FACTORIZATION)
            active_users[(user, this_user)] = (p, g)
            return f'{p},{g}'

    elif request['method'] == 'POST':
        return 'Not implemented yet!'
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

if __name__ == '__main__':
    print(find_generator(SAFE_PRIME, FACTORIZATION))
