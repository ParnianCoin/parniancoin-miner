/**
 * Generic command line parser
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 */

///
/// @file
/// @copyright Amir Reza Zamani <amirrezazamani@gmail.com>
/// @author Amir Reza Zamani <amirrezazamani@gmail.com>
#pragma once
#include <json/json.h>


#include "corelib/basetypes.h" 
#include <condition_variable>
#include "FixedHash.h"
#include "utils.h"


//Randomize nonce2
#define RH_RANDOMIZE_NONCE2

struct ParnianSolution;

struct ParnianWorkPackage
{
    ParnianWorkPackage();
    explicit ParnianWorkPackage(bool isSolo) { m_isSolo = isSolo; }
    explicit ParnianWorkPackage(const ParnianWorkPackage& c);
    ~ParnianWorkPackage();
    ParnianWorkPackage* Clone();

    void    Init(const string& job, const h256& prevHash, const string& coinbase1, const string& coinbase2, const string& nTime, bool cleanWork, const string& nonce1, U32 nonce2Size, U64 extranonce, const string& server);
    bool    Eval(ParnianSolution* solPtr);
    void    ComputeWorkDiff(double& diff);
    bool    IsSame(ParnianWorkPackage* work);
    U32     GetDeviceTargetUpperBits();
    U64     GetDeviceTargetUpperBits64();
    void    UpdateHeader();
    void    ComputeTargetBoundary();
    bool    IsEmpty();
    h256    RebuildNonce(U64 nonce);
    string  ComputePayload();

    static void     ComputeTargetBoundary(h256& boundary, double& diff, double diffMultiplyer);
    static U64	    ComputeNonce2(U32 nonce2);

    string          m_jobID;
    string          m_ntime;   
    U64             m_initTimeMS = 0;
    h256            m_prev_hash;
    U64             m_startNonce = 0; 
    mutable h256    m_boundary;                
    mutable h256    m_deviceBoundary;
    mutable h256    m_soloTargetPow;
    double          m_workDiff = 1.0;          
    double          m_deviceDiff = 1.0;        
    bool            m_localyGenerated = false; 
    U32             m_nonce2 = U32_Max;
    bytes           m_fullHeader;
    string          m_coinbase1;
    string          m_coinbase2;
    string          m_nonce1;
    unsigned        m_nonce2Size = 0; 
    bool            m_clean = false;
    U64             m_nonce2_64 = 0;
    string          m_server;
    std::set<U64>   m_submittedNonces;

    //solo stuff
    bool            m_isSolo = false;
};
typedef std::shared_ptr<ParnianWorkPackage> ParnianWorkSptr;

struct ParnianSolution
{
    U64                 GetCurrentEvaluatingNonce();
    void                SetCurrentEvaluatingNonceIndex(U32 i);
    bool                Eval() { return m_work->Eval(this);  }
        
    h256                m_calcHash;
    std::vector<U64>    m_results;
    U32                 m_gpuIndex = 0;
    bool                m_isFromCpuMiner = false;
    ParnianWorkSptr      m_work;

private:
    U32                 _eval_current_result_index = 0; //index used to travers m_results and eval each value

};
typedef std::shared_ptr<ParnianSolution> SolutionSptr;

