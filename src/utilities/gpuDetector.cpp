/*
    DETECTS GPUS and assigns each to MPI process.
*/

#include "gpuDetector.h"

#include <cstring>
#include <iostream>
#include <string>

#include <cuda.h>
#include <cuda_runtime.h>
#include <mpi.h>
#include <unistd.h>

// There's a memory leak in cudaGetDeviceByPciBusID so avoid it and assign simply
// unitl it's fixed. This makes most of the code here useless.
#define PCIBUS_BUG 1

int getHost(hvec &ids, hname *newHost)
{
    char machineName[RLEN];
    int rb = RLEN;
    int nGo = 0;
    cudaGetDeviceCount(&nGo);
    MPI_Get_processor_name(&machineName[0],  &rb);
    for (int i=0; i<ids.size(); i++)
    {
        if (!std::strcmp(ids[i].hostname, machineName))
        {
            return i;
        }
    }

    std::strcpy(newHost->hostname, machineName);
    newHost->ng = nGo;
    return ids.size();
}

bool detector(const int ranko, const int sz, const int startpos)
{
    hvec ledger;
    int machineID;
    int hasG = false;

	hname hBuf;
    MPI_Datatype atype;
    MPI_Datatype typs[] = {MPI_INT, MPI_CHAR};
    int nm[] = {1, RLEN};
    MPI_Aint disp[] = {0, 4};
    MPI_Type_create_struct(2, nm, disp, typs, &atype);
    MPI_Type_commit(&atype);

    for (int k=0; k<sz; k++)
    {
        if (ranko == k)
        {
            machineID = getHost(ledger, &hBuf);
        }
        //Broadcast the updated host list - vis GPUs to all procs.
        MPI_Bcast(&hBuf, 1, atype, k, MPI_COMM_WORLD);
		if (ledger.size() > 0)
		{
			if (strcmp(hBuf.hostname, ledger.back().hostname))
			{
				ledger.push_back(hBuf);
			}
		}
		else
		{
			ledger.push_back(hBuf);
		}
    }
    MPI_Barrier(MPI_COMM_WORLD);

	MPI_Comm machineComm;
    MPI_Comm_split(MPI_COMM_WORLD, machineID, ranko, &machineComm);
    int machineRank, machineSize;
    MPI_Comm_rank(machineComm, &machineRank);
    MPI_Comm_size(machineComm, &machineSize);

    MPI_Barrier(MPI_COMM_WORLD);

	int nGo = ledger[machineID].ng;
    int pcivec[nGo*3];
	cudaDeviceProp props;

	MPI_Barrier(MPI_COMM_WORLD);

	if (machineRank == 0)
    {
        for (int k = 0; k < nGo; k++)
        {
            cudaGetDeviceProperties(&props, k);

			pcivec[3*k] = props.pciDomainID;
			pcivec[3*k+1] = props.pciBusID;
			pcivec[3*k+2] = props.pciDeviceID;
    	}
    }

	MPI_Bcast(&pcivec[0], 3*nGo, MPI_INT, 0, machineComm);
	MPI_Barrier(MPI_COMM_WORLD);

	int nset = 0;
    int dev;
	std::string pcistr;
    int rb = 30;
	char bufs[rb];
    char mnamei[rb];
    char *mname;

    for (int i = startpos; i<machineSize; i++)
    {
        if ((nGo - nset) == 0)  break;
        if (i == machineRank)
        {
            sprintf(bufs, "%x:%x:%x", pcivec[3*nset],pcivec[3*nset+1],pcivec[3*nset+2]);
#ifndef PCIBUS_BUG
            cudaDeviceGetByPCIBusId(&dev, bufs);
#else
            dev=i-1;
#endif
            cudaGetDeviceProperties(&props, dev);

            if (!props.kernelExecTimeoutEnabled)
            {
                cudaSetDevice(dev);
           	    MPI_Get_processor_name(&mnamei[0],  &rb);
                mname=strtok(mnamei, ".");

                hasG = true;
                std::cout << "PID: " << getpid() << " - Rank: " << ranko;
                std::cout << " - Machine: " << machineID << "- " << mname << " - Has GPU: ";
                std::cout << dev << " of " << nGo << "/" << nset << " on PCI: " << bufs << std::endl;
            }
            nset++;
		}
        MPI_Bcast(&nset, 1, MPI_INT, i, machineComm);
        MPI_Barrier(machineComm);
    }

    MPI_Type_free(&atype);
    MPI_Comm_free(&machineComm);
    MPI_Barrier(MPI_COMM_WORLD);
    if(!ranko) std::cout << "GPUS HAVE BEEN CHOSEN ---------------------- " << std::endl;
    return hasG;
}
