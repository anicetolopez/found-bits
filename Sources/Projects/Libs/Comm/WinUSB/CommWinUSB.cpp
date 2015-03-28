/////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, pa.eeapai@gmail.com                                         //
// All rights reserved.                                                            //
//                                                                                 //
// Redistribution and use in source and binary forms, with or without              //
// modification, are permitted provided that the following conditions are met:     //
//     * Redistributions of source code must retain the above copyright            //
//       notice, this list of conditions and the following disclaimer.             //
//     * Redistributions in binary form must reproduce the above copyright         //
//       notice, this list of conditions and the following disclaimer in the       //
//       documentation and/or other materials provided with the distribution.      //
//     * Neither the name of the pa.eeapai@gmail.com nor the                       //
//       names of its contributors may be used to endorse or promote products      //
//       derived from this software without specific prior written permission.     //
//                                                                                 //
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND //
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   //
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          //
// DISCLAIMED. IN NO EVENT SHALL pa.eeapai@gmail.com BE LIABLE FOR ANY             //
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES      //
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;    //
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND     //
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT      //
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS   //
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                    //
/////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CommWinUSB.h"

CCommWinUSB::CCommWinUSB(SCommLayer *psCommLayer) :
  base_class(psCommLayer),
  m_pcszDevicePath(NULL)
{
  memset(m_achDevicePath, 0, sizeof(m_achDevicePath));
}
CCommWinUSB::~CCommWinUSB()
{

}

void CCommWinUSB::Configure(LPCSTR pcszPath, BYTE *pbyBuff, COMMCOUNT cntBufferSize)
{
  m_pcszDevicePath = NULL;
  if ( pcszPath )
  {
    memset(m_achDevicePath, 0, sizeof(m_achDevicePath));
    memcpy_s(m_achDevicePath, sizeof(m_achDevicePath), pcszPath, strlen(pcszPath));
    m_pcszDevicePath = m_achDevicePath;
  }
  base_class::Configure(pbyBuff, cntBufferSize);
}

void CCommWinUSB::ClientInit()
{
  base_class::ClientInit();
  if ( !m_USBDev.Connect(m_pcszDevicePath) )
  {
    m_psCommLayer->m_hCommStack->m_byLastError = commstatusError;
  }
}

void CCommWinUSB::PacketStart()
{
  base_class::PacketStart();
  HCOMMSTACK hComm = m_psCommLayer->m_hCommStack;
  const char *pcszDest = hComm->m_pcszDestination;
  if ( !pcszDest )
  {
    pcszDest = "";
  }
  CommStack_Send(hComm, (BYTE *)pcszDest, strlen(pcszDest) + 1);
  CommStack_Send(hComm, &hComm->m_byOP, sizeof(hComm->m_byOP));
  CommStack_Send(hComm, &hComm->m_byI, sizeof(hComm->m_byI));
  CommStack_Send(hComm, &hComm->m_byMoP, sizeof(hComm->m_byMoP));
  CommStack_Send(hComm, &hComm->m_byIPI, sizeof(hComm->m_byIPI));
}

void CCommWinUSB::PacketEnd()
{
  if ( m_USBDev.SendData(GetBuffer(), GetSendSize()) )
  {
    return;
  }
  m_psCommLayer->m_hCommStack->m_byLastError = commstatusError;
}

ECommStatus CCommWinUSB::TransmitProcess()
{
  ECommStatus eStatus = base_class::TransmitProcess();
  if ( commstatusIdle != eStatus )
  {
    return eStatus;
  }
  return (ECommStatus)m_psCommLayer->m_hCommStack->m_byLastError;
}
ECommStatus CCommWinUSB::ReceiveProcess(COMMCOUNT *pcntByteCount, unsigned char **ppbyReceivedData)
{
  ECommStatus eStatus = base_class::ReceiveProcess(pcntByteCount, ppbyReceivedData);
  if ( commstatusIdle != eStatus )
  {
    return eStatus;
  }

  if ( pcntByteCount )
  {
    *pcntByteCount = 0;
  }

  if ( commstatusIdle != m_psCommLayer->m_hCommStack->m_byLastError )
  {
    return commstatusError;
  }

  DWORD dwResponseSize = 0;
  if ( !m_USBDev.CanReceive(dwResponseSize) )
  {
    return commstatusError;
  }
  if ( dwResponseSize )
  {
    if ( dwResponseSize > GetBufferSize() )
    {
      return commstatusOverrun;
    }
    
    if ( !m_USBDev.DoReceive(GetBuffer(), dwResponseSize) )
    {
      return commstatusError;
    }
    OnNewPacketNotify();
    BYTE *pbyData = GetBuffer();
    OnDataNotify(pbyData, dwResponseSize);
    OnPacketEndNotify();


    DWORD dwHeaderLength = 0;
//     HCOMMSTACK hComm = GetStack();
//     hComm->m_pcszDestination = (char *)pbyData;
//     dwHeaderLength += strlen(hComm->m_pcszDestination) + 1;
//     hComm->m_byOP = pbyData[dwHeaderLength];
//     dwHeaderLength += sizeof(hComm->m_byOP);
//     hComm->m_byI = pbyData[dwHeaderLength];
//     dwHeaderLength += sizeof(hComm->m_byI);
//     hComm->m_byMoP = pbyData[dwHeaderLength];
//     dwHeaderLength += sizeof(hComm->m_byMoP);
//     hComm->m_byIPI = pbyData[dwHeaderLength];
//     dwHeaderLength += sizeof(hComm->m_byIPI);

    *ppbyReceivedData = GetBuffer() + dwHeaderLength;
    *pcntByteCount = (COMMCOUNT)GetReceivedSize() - dwHeaderLength;
    return commstatusNewPacket;
  }
  return commstatusIdle;
}
void CCommWinUSB::Disconnect()
{
  base_class::Disconnect();  
  m_USBDev.Disconnect();
}

