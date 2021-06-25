from pippi import dsp, shapes

dsp.seed(111)

minval = 0.
maxval = 1.
grainsperbranch = 4

passes = (4, 8, 12, 18, 40, 400)

for numgrains in passes:
    A = dsp.rand(minval, maxval)
    B = dsp.rand(minval, maxval)

    numbranches = numgrains // grainsperbranch

    if A > B:
        A, B = B, A

    trunk = dsp.win('rsaw', A, B)
    trunk.graph('trunk-%s-rsaw.png' % numgrains, y=(0,1))

    branches = []
    for _ in range(numbranches):
        bD = dsp.rand(0.001, 0.999) # delta
        bA = dsp.rand(max(A - (bD/2), 0), min(A + (bD/2), 1))
        branches += [ dsp.buffer(dsp.win('rsaw', bA, B), channels=1) ]

    branches = dsp.stack(branches)
    branches.graph('branches-%s-rsaw.png' % numgrains, y=(0,1))

    curve = shapes.win('hann', length=0.1)
    trunk = dsp.win(curve, A, B)
    trunk.graph('trunk-%s-randhann.png' % numgrains, y=(0,1))

    branches = []
    for _ in range(numbranches):
        bD = dsp.rand(0.001, 0.999) # delta
        bA = dsp.rand(max(A - (bD/2), 0), min(A + (bD/2), 1))
        branches += [ dsp.buffer(dsp.win(curve, bA, B), channels=1) ]

    branches = dsp.stack(branches)
    branches.graph('branches-%s-randhann.png' % numgrains, y=(0,1))


