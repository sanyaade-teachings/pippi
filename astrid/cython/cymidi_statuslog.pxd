cdef extern from "astrid.h":
    int lpmidi_setcc(int device_id, int cc, int value)
    int lpmidi_getcc(int device_id, int cc)
    int lpmidi_setnote(int device_id, int note, int velocity)
    int lpmidi_getnote(int device_id, int note)
    int lpmidi_trigger_notemap(int device_id, int note)


cpdef int getcc(int cc=*, int device_id=*)
cpdef int setcc(int cc=*, int value=*, int device_id=*)
cpdef int getnote(int note=*, int device_id=*)
cpdef int setnote(int note=*, int velocity=*, int device_id=*)
cpdef int trigger_notemap(int note=*, int device_id=*)
 
