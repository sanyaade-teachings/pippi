cpdef int getcc(int cc=0, int device_id=0):
    return lpmidi_getcc(device_id, cc)

cpdef int setcc(int cc=0, int value=0, int device_id=0):
    return lpmidi_setcc(device_id, cc, value)

cpdef int getnote(int note=60, int device_id=0):
    return lpmidi_getnote(device_id, note)

cpdef int setnote(int note=60, int velocity=0, int device_id=0):
    return lpmidi_setnote(device_id, note, velocity)

cpdef int trigger_notemap(int note=60, int device_id=0):
    return lpmidi_trigger_notemap(device_id, note)
