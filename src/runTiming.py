'''
    Run experiment.
'''
#%%
import os
import os.path as op
import sys
import pandas as pd
import numpy as np
import subprocess as sp
import shlex

thispath = op.abspath(op.dirname(__file__))
os.chdir(thispath)
toppath = op.dirname(thispath)
pypath = op.join(toppath, "runtools")

sys.path.append(pypath)

from main_help import *
import result_help as rh
import timing_help as th

ext = ".json"

eq = ["heat", "euler"];

#makr = "nvcc solmain.cu jsoncpp.cpp -o ./bin/euler -gencode arch=compute_35,code=sm_35 -O3 -restrict -std=c++11 -I/usr/include/mpi -lmpi -Xcompiler -fopenmp -lm -w --ptxas-options=-v"
#makr = shlex.split(makr)

#%%
nproc = 8
mpiarg = "" #"--bind-to socket
tstrng = os.listdir(testpath)
schemes = ["S", "C"]
schD = {schemes[0]: "Classic", schemes[1]: "Swept"}

#if op.isfile(prog):
#
#    sgrp = os.stat(prog)
#    spp = sgrp.st_mtime
#    prer = os.listdir(thispath)
#    suff = [".h", "cpp", ".cu", ".sh"]
#    prereq = [k for k in prer for n in suff if n in k]
#
#    ftim = []
#    for p in prereq:
#        fg = os.stat(p)
#        ftim.append(fg.st_mtime)
#
#    ts = sum([fo > spp for fo in ftim])
#    if ts:
#        print("Making executable...")
#        proc = sp.Popen(makr)
#        sp.Popen.wait(proc)
#
#else:
#    print("Making executable...")
#    proc = sp.Popen(makr)
#    sp.Popen.wait(proc)

#%%
#Say iterate over gpuA at one size and tpb
gpus = [k/1.5 for k in range(1, 15)] #GPU Affinity
nX = [2**k for k in range(12,21)] #Num spatial pts (Grid Size)
tpb = [2**k for k in range(6,10)]
ac = 1

#%%
for p in eq:
    prog = op.join(binpath, p)
    prog += " "
    eqs = [k for k in tstrng if p.upper() in k.upper()]
    eqspec = op.join(testpath, eqs[0])
    eqspec += " "
    for sc in schemes:
        timeTitle = "t"+p.title()+sc+ext
        print(timeTitle)

        eqn = p + " " + schD[sc]
        print(eqn)
        if not ac:
            ac += 1
            break

        sc += " "

        for t in tpb:
            for n in nX:
                xl = int(n/10000) + 1
                for g in gpus:
                    exargs =  " freq 200 gpuA {:.4f} nX {:d} tpb {:d} lx {:d}".format(g, n, t, xl)
                    runMPICUDA(prog, nproc, sc, eqspec, mpiopt=mpiarg, eqopt=exargs, outdir=rspath)

        tfile = op.join(rspath, timeTitle)
        res = th.Perform(tfile)
        res.plotdict(eqn, plotpath=resultpath)