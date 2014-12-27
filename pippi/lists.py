from pippi import rand

def interleave(list_one, list_two):
    """ Combine two lists by interleaving their elements """
    # find the length of the longest list
    if len(list_one) > len(list_two):
        big_list = len(list_one)
    elif len(list_two) > len(list_one):
        big_list = len(list_two)
    else:
        if rand.randint(0, 1) == 0:
            big_list = len(list_one)
        else:
            big_list = len(list_two)

    combined_lists = []

    # loop over it and insert alternating items
    for index in range(big_list):
        if index <= len(list_one) - 1:
            combined_lists.append(list_one[index])
        if index <= len(list_two) - 1:
            combined_lists.append(list_two[index])

    return combined_lists

def packet_shuffle(list, packet_size):
    """ Shuffle a subset of list items in place.
        Takes a list, splits it into sub-lists of size N
        and shuffles those sub-lists. Returns flattened list.
    """
    if packet_size >= 3 and packet_size <= len(list):
        lists = list_split(list, packet_size)
        shuffled_lists = []
        for sublist in lists:
            shuffled_lists.append(rand.randshuffle(sublist))

        big_list = []
        for shuffled_list in shuffled_lists:
            big_list.extend(shuffled_list)
        return big_list

def list_split(list, packet_size):
    """ Split a list into groups of size N """
    trigs = []
    for i in range(len(list)):
        if i % int(packet_size) == 0:
            trigs.append(i)

    newlist = []

    for trig_index, trig in enumerate(trigs):
        if trig_index < len(trigs) - 1:
            packets = []
            for packet_bit in range(packet_size):
                packets.append(list[packet_bit + trig])

            newlist.append(packets)

    return newlist

def rotate(list, start=0, rand=False):
    """ Rotate a list by a given offset """

    if rand == True:
        start = rand.randint(0, len(list) - 1)

    if start > len(list) - 1:
        start = len(list) - 1

    return list[start:] + list[:start]

def eu(length, numpulses):
    """ Euclidian pattern generator
    """
    pulses = [ 1 for pulse in range(numpulses) ]
    pauses = [ 0 for pause in range(length - numpulses) ]

    position = 0
    while len(pauses) > 0:
        try:
            index = pulses.index(1, position)
            pulses.insert(index + 1, pauses.pop(0))
            position = index + 1
        except ValueError:
            position = 0

    return pulses


