import hashlib
import random

seedint = 0
seedstep = 0
seedhash = ''

def seed(theseed=False):
    global seedint
    global seedhash

    if theseed == False:
        theseed = cycle(440)

    h = hashlib.sha1(theseed)
    seedhash = h.digest()

    seedint = int(''.join([str(ord(c)) for c in list(seedhash)]))
    return seedint

def stepseed():
    global seedint
    global seedstep

    h = hashlib.sha1(str(seedint))
    seedint = int(''.join([str(ord(c)) for c in list(h.digest())]))

    seedstep = seedstep + 1

    return seedint

def randint(lowbound=0, highbound=1):
    return int(round(rand() * (highbound - lowbound) + lowbound))

def rand(lowbound=0, highbound=1):
    global seedint
    if seedint > 0:
        return ((stepseed() / 100.0**20) % 1.0) * (highbound - lowbound) + lowbound
    else:
        return random.random() * (highbound - lowbound) + lowbound

def randchoose(items):
    return items[randint(0, len(items)-1)]

def randshuffle(input):
    items = input[:]
    shuffled = []
    for i in range(len(items)):
        if len(items) > 0:
            item = randchoose(items)
            shuffled.append(item)
            items.remove(item)

    return shuffled 

def randbool():
    return randchoose([True, False])

def markov(state, chain, values=None):
    """ Pass a dict in this format: 

        chain = {
            'a': [0.1, 1, 0.5],
            'b': [0, 0.2, 0.75],
            'c': [0, 1],
        }

        Optionally pass a dict to return a 
        preselected value or set of values:

        values = {
            'a': (1, 0.75),
            'b': (0, 400),
            'c': (20, 0.35)
        }

        Otherwise the state name will be returned.
    """
    
    states = []
    weights = []

    for s, w in chain.iteritems():
        states += [s]
        weights += [w]

    if state in states:
        weight = chain[state]
    else:
        weight = chain[randchoose(states)]

    choices = []

    for i, s in enumerate(states):
        try:
            choices += [s] * int(weight[i] * 100)
        except IndexError:
            pass

    state = randchoose(choices)
 
    if values is not None and state in values:
        value = values[state]
    else:
        value = state

    return value
