##  std includes

##  TYPES

when defined(LP_FLOAT):
  type
    LpfloatT* = cfloat
else:
  type
    LpfloatT* = cdouble
##  CONSTANTS

when not defined(PI):
  const
    PI* = 3.1415926535897932384626433832795028841971693993751058209749445923078164062
when not defined(PI2):
  const
    PI2* = (pi * 2.0)
const
  SINE* = "sine"
  SQUARE* = "square"
  TRI* = "tri"
  PHASOR* = "phasor"
  HANN* = "hann"
  RND* = "rnd"
  DEFAULT_CHANNELS* = 2
  DEFAULT_SAMPLERATE* = 48000

##  Core datatypes

type
  BufferT* {.bycopy.} = object
    data*: ptr LpfloatT
    length*: csize_t
    samplerate*: cint
    channels*: cint            ##  used for different types of playback
    phase*: LpfloatT
    pos*: csize_t


##  Factories & static interfaces

type
  LprandT* {.bycopy.} = object
    seed*: proc (a1: cint)
    rand*: proc (a1: LpfloatT; a2: LpfloatT): LpfloatT
    randint*: proc (a1: cint; a2: cint): cint
    randbool*: proc (): cint
    choice*: proc (a1: cint): cint

  BufferFactoryT* {.bycopy.} = object
    create*: proc (a1: csize_t; a2: cint; a3: cint): ptr BufferT
    scale*: proc (a1: ptr BufferT; a2: LpfloatT; a3: LpfloatT; a4: LpfloatT; a5: LpfloatT)
    play*: proc (a1: ptr BufferT; a2: LpfloatT): LpfloatT
    mix*: proc (a1: ptr BufferT; a2: ptr BufferT): ptr BufferT
    multiply*: proc (a1: ptr BufferT; a2: ptr BufferT)
    dub*: proc (a1: ptr BufferT; a2: ptr BufferT)
    env*: proc (a1: ptr BufferT; a2: ptr BufferT)
    destroy*: proc (a1: ptr BufferT)

  ParamFactoryT* {.bycopy.} = object
    fromFloat*: proc (a1: LpfloatT): ptr BufferT
    fromInt*: proc (a1: cint): ptr BufferT


##  Users may create custom memorypools.
##  If the primary memorypool is active,
##  it will be used to allocate the pool.
##
##  Otherwise initializtion of the pool
##  will use the stdlib to calloc the space.
##

type
  MemorypoolT* {.bycopy.} = object
    pool*: ptr cuchar
    poolsize*: csize_t
    pos*: csize_t

  MemorypoolFactoryT* {.bycopy.} = object
    pool*: ptr cuchar           ##  This is the primary memorypool.
    poolsize*: csize_t
    pos*: csize_t
    init*: proc (a1: ptr cuchar; a2: csize_t)
    customInit*: proc (a1: ptr cuchar; a2: csize_t): ptr MemorypoolT
    alloc*: proc (a1: csize_t; a2: csize_t): pointer
    customAlloc*: proc (a1: ptr MemorypoolT; a2: csize_t; a3: csize_t): pointer
    free*: proc (a1: pointer)

  interpolationFactoryT* {.bycopy.} = object
    linearPos*: proc (a1: ptr BufferT; a2: LpfloatT): LpfloatT
    linear*: proc (a1: ptr BufferT; a2: LpfloatT): LpfloatT
    hermitePos*: proc (a1: ptr BufferT; a2: LpfloatT): LpfloatT
    hermite*: proc (a1: ptr BufferT; a2: LpfloatT): LpfloatT

  WavetableFactoryT* {.bycopy.} = object
    create*: proc (name: cstring; length: csize_t): ptr BufferT
    destroy*: proc (a1: ptr BufferT)

  WindowFactoryT* {.bycopy.} = object
    create*: proc (name: cstring; length: csize_t): ptr BufferT
    destroy*: proc (a1: ptr BufferT)


##  Interfaces

var rand*: LprandT

var buffer*: BufferFactoryT

var memoryPool*: MemorypoolFactoryT

var interpolation*: interpolationFactoryT

var param*: ParamFactoryT

var wavetable*: WavetableFactoryT

var window*: WindowFactoryT

##  Utilities
##  The zapgremlins() routine was written by James McCartney as part of SuperCollider:
##  https://github.com/supercollider/supercollider/blob/f0d4f47a33b57b1f855fe9ca2d4cb427038974f0/headers/plugin_interface/SC_InlineUnaryOp.h#L35
##
##  SuperCollider real time audio synthesis system
##  Copyright (c) 2002 James McCartney. All rights reserved.
##  http://www.audiosynth.com
##
##  He says:
##       This is a function for preventing pathological math operations in ugens.
##       It can be used at the end of a block to fix any recirculating filter values.
##

proc zapgremlins*(x: LpfloatT): LpfloatT