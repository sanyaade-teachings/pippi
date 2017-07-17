def parse_cents(tuning):
    try:
        return 2**(float(tuning) / 1200)
    except (TypeError, ValueError):
        return None

def parse_ratio(tuning):
    if '/' in tuning:
        try:
            ratio = [ t for t in tuning.split('/') if t != '' ][:2]
            return int(ratio[0])/int(ratio[1])
        except (TypeError, ValueError):
            return None

    try:
        return int(tuning)
    except (TypeError, ValueError):
        return None

def import_file(filename):
    with open(filename) as tuning_file:
        description = None
        tunings = [1]
        for line in tuning_file:
            if line[0] == '!':
                continue

            if description is None:
                description = line.strip()
                continue

            tuning = line.strip().split(' ')[0]
            if '.' in tuning:
                tuning = parse_cents(tuning)
            else:
                tuning = parse_ratio(tuning)

            if tuning is None:
                continue

            tunings += [ tuning ]

    return tunings

