/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "net-device.h"
#include <iostream>
#include <vector>

NS_LOG_COMPONENT_DEFINE ("NetDevice");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (NetDevice);

TypeId NetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetDevice")
    .SetParent<Object> ()
  ;
  return tid;
}
NetDevice::~NetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

uint32_t NetDevice::GetUsedBuffer(uint32_t port, uint32_t qIndex)
{
	//std::cout<<"WARNING: shouldn't reach here --- net-device.cc\n";
	return 0;
}

void NetDevice::FlowFinished(uint32_t port, uint32_t pg, uint32_t lastseq)
{
	//std::cout<<"WARNING: shouldn't reach here --- net-device.cc\n";
//return 0;
}

void NetDevice::SendPauseFrame(std::vector<int> pClasses[]){//uint64_t pause_map_a, uint64_t pause_map_b) {
}


void NetDevice::UpdatePauseQueue(uint32_t qIndex, uint32_t p_hqCnt)
{

}
void NetDevice::UpdateResumeQueue(uint32_t qIndex, uint32_t p_hqCnt)
{

}

}
