from pippi import rand

def interleave(list_one, list_two):
    """ Interleave the elements of two lists.

        ::

            >>> dsp.interleave([1,1], [0,0])
            [1, 0, 1, 0]
    
    """
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

def packet_shuffle(items, size):
    """ Shuffle the items in a list at a given granularity >= 3

        ::

            >>> dsp.packet_shuffle(range(10), 3)
            [0, 1, 2, 5, 4, 3, 8, 7, 6]
    
            >>> dsp.packet_shuffle(range(10), 3)
            [1, 2, 0, 4, 3, 5, 7, 6, 8]

    """
    if size >= 3 and size <= len(items):
        lists = list_split(items, size)
        shuffled_lists = []
        for sublist in lists:
            shuffled_lists.append(rand.randshuffle(sublist))

        big_list = []
        for shuffled_list in shuffled_lists:
            big_list.extend(shuffled_list)

        if len(items) > len(big_list):
            spill_length = len(items) - len(big_list)
            big_list += items[:spill_length]

        return big_list

    else:
        return items

def list_split(items, packet_size):
    """ Split a list into lists of a given size
    
        ::

            >>> dsp.list_split(range(10), 3)
            [[0, 1, 2], [3, 4, 5], [6, 7, 8]]

    """
    trigs = []
    for i in range(len(items)):
        if i % int(packet_size) == 0:
            trigs.append(i)

    newlist = []

    for trig_index, trig in enumerate(trigs):
        if trig_index < len(trigs) - 1:
            packets = []
            for packet_bit in range(packet_size):
                packets.append(items[packet_bit + trig])

            newlist.append(packets)

    return newlist

def rotate(items, start=0, vary=False):
    """ Rotate a list by a given (optionally variable) offset 
    
        :: 

            >>> dsp.rotate(range(10), 3)
            [3, 4, 5, 6, 7, 8, 9, 0, 1, 2]

            >>> dsp.rotate(range(10), vary=True)
            [6, 7, 8, 9, 0, 1, 2, 3, 4, 5]

            >>> dsp.rotate(range(10), vary=True)
            [8, 9, 0, 1, 2, 3, 4, 5, 6, 7]
    
    """

    if vary == True:
        start = rand.randint(0, len(items) - 1)

    if start > len(items) - 1:
        start = len(items) - 1

    return items[start:] + items[:start]

def eu(length, numpulses):
    """ A euclidian pattern generator

        ::

            >>> dsp.eu(12, 3)
            [1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0]

            >>> dsp.eu(12, 5)
            [1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0]
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


