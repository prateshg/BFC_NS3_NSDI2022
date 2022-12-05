/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 New York University
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
 * Author: Adrian S.-W. Tam <adrian.sw.tam@gmail.com>
 */

#include <stdint.h>
#include <iostream>
#include "pause-header.h"
#include "ns3/buffer.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("PauseHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PauseHeader);

PauseHeader::PauseHeader (uint32_t time, uint16_t num, uint16_t pause_map[])
  : m_time(time)
{
	m_num = num;
	for (int i = 0; i < m_num; i++) {
		m_pause_map[i] = pause_map[i];
	}
  /*m_pause_map_a = pause_map[0];
  m_pause_map_b = pause_map[1];
  m_pause_map_c = pause_map[2];
  m_pause_map_d = pause_map[3];
  m_pause_map_e = pause_map[4];
  m_pause_map_f = pause_map[5];
  m_pause_map_g = pause_map[6];
  m_pause_map_h = pause_map[7];
  m_pause_map_i = pause_map[8];
  m_pause_map_j = pause_map[9];
  m_pause_map_k = pause_map[10];
  m_pause_map_l = pause_map[11];
  m_pause_map_m = pause_map[12];
  m_pause_map_n = pause_map[13];
  m_pause_map_o = pause_map[14];
  m_pause_map_p = pause_map[15];*/
}

PauseHeader::PauseHeader ()
  : m_time(0)
{
	m_num = 0;
}

PauseHeader::~PauseHeader ()
{}

void PauseHeader::SetTime (uint32_t time)
{
  m_time = time;
}

uint32_t PauseHeader::GetTime () const
{
  return m_time;
}

/*void PauseHeader::SetPMap (uint16_t num, uint16_t pause_map[])
{
	m_num = num;
	for (uint16_t i = 0; i < m_num; i++) {
		m_pause_map[i] = pause_map[i];
	}
  m_pause_map_a = pause_map[0];
  m_pause_map_b = pause_map[1];
  m_pause_map_c = pause_map[2];
  m_pause_map_d = pause_map[3];
  m_pause_map_e = pause_map[4];
  m_pause_map_f = pause_map[5];
  m_pause_map_g = pause_map[6];
  m_pause_map_h = pause_map[7];
  m_pause_map_i = pause_map[8];
  m_pause_map_j = pause_map[9];
  m_pause_map_k = pause_map[10];
  m_pause_map_l = pause_map[11];
  m_pause_map_m = pause_map[12];
  m_pause_map_n = pause_map[13];
  m_pause_map_o = pause_map[14];
  m_pause_map_p = pause_map[15];
}*/

uint16_t PauseHeader::GetNum () const
{
	return m_num;
}

void PauseHeader::GetPMap (uint16_t pause_map[])
{
	for (uint16_t i = 0; i < m_num; i++) {
		pause_map[i] = m_pause_map[i];
	}
	/*pause_map[0] = m_pause_map_a;
	pause_map[1] = m_pause_map_b;
	pause_map[2] = m_pause_map_c;
	pause_map[3] = m_pause_map_d;
	pause_map[4] = m_pause_map_e;
	pause_map[5] = m_pause_map_f;
	pause_map[6] = m_pause_map_g;
	pause_map[7] = m_pause_map_h;
	pause_map[8] = m_pause_map_i;
	pause_map[9] = m_pause_map_j;
	pause_map[10] = m_pause_map_k;
	pause_map[11] = m_pause_map_l;
	pause_map[12] = m_pause_map_m;
	pause_map[13] = m_pause_map_n;
	pause_map[14] = m_pause_map_o;
	pause_map[15] = m_pause_map_p;*/
}

/*void PauseHeader::SetPMapB (uint64_t pause_map_b)
{
  m_pause_map_b = pause_map_b;
}

uint64_t PauseHeader::GetPMapB () const
{
  return m_pause_map_b;
}*/

/*void PauseHeader::SetQIndex (uint8_t qindex)
{
  m_qindex = qindex;
}

uint8_t PauseHeader::GetQIndex () const
{
  return m_qindex;
}*/

TypeId
PauseHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PauseHeader")
    .SetParent<Header> ()
    .AddConstructor<PauseHeader> ()
    ;
  return tid;
}
TypeId
PauseHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void PauseHeader::Print (std::ostream &os) const
{
  //os << "pause=" << m_time << "us, pause_map_a=" << m_pause_map_a <<", m_pause_map_b=" << m_pause_map_b;
}
uint32_t PauseHeader::GetSerializedSize (void)  const
{
  return 6 + 2 * m_num;
  //return 132;
}
void PauseHeader::Serialize (Buffer::Iterator start)  const
{
	start.WriteHtonU32 (m_time);
  	start.WriteHtonU16 (m_num);
	for (int i = 0; i < m_num; i++) {
		start.WriteHtonU16 (m_pause_map[i]);
	}
  /*start.WriteHtonU64 (m_pause_map_a);
  start.WriteHtonU64 (m_pause_map_b);
  start.WriteHtonU64 (m_pause_map_c);
  start.WriteHtonU64 (m_pause_map_d);
  start.WriteHtonU64 (m_pause_map_e);
  start.WriteHtonU64 (m_pause_map_f);
  start.WriteHtonU64 (m_pause_map_g);
  start.WriteHtonU64 (m_pause_map_h);
  start.WriteHtonU64 (m_pause_map_i);
  start.WriteHtonU64 (m_pause_map_j);
  start.WriteHtonU64 (m_pause_map_k);
  start.WriteHtonU64 (m_pause_map_l);
  start.WriteHtonU64 (m_pause_map_m);
  start.WriteHtonU64 (m_pause_map_n);
  start.WriteHtonU64 (m_pause_map_o);
  start.WriteHtonU64 (m_pause_map_p);*/

}

uint32_t PauseHeader::Deserialize (Buffer::Iterator start)
{
	m_time = start.ReadNtohU32 ();
	m_num = start.ReadNtohU16 ();
	for (uint16_t i = 0; i < m_num; i++) {
		m_pause_map[i] = start.ReadNtohU16 ();
	}
  /*m_pause_map_a = start.ReadNtohU64 ();
  m_pause_map_b = start.ReadNtohU64 ();
  m_pause_map_c = start.ReadNtohU64 ();
  m_pause_map_d = start.ReadNtohU64 ();
  m_pause_map_e = start.ReadNtohU64 ();
  m_pause_map_f = start.ReadNtohU64 ();
  m_pause_map_g = start.ReadNtohU64 ();
  m_pause_map_h = start.ReadNtohU64 ();
  m_pause_map_i = start.ReadNtohU64 ();
  m_pause_map_j = start.ReadNtohU64 ();
  m_pause_map_k = start.ReadNtohU64 ();
  m_pause_map_l = start.ReadNtohU64 ();
  m_pause_map_m = start.ReadNtohU64 ();
  m_pause_map_n = start.ReadNtohU64 ();
  m_pause_map_o = start.ReadNtohU64 ();
  m_pause_map_p = start.ReadNtohU64 ();*/
  return GetSerializedSize ();
}


}; // namespace ns3
