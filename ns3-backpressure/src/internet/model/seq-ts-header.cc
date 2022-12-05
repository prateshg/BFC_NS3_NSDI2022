/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "seq-ts-header.h"

NS_LOG_COMPONENT_DEFINE ("SeqTsHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SeqTsHeader);

SeqTsHeader::SeqTsHeader ()
  : m_seq (0),
    m_ts (Simulator::Now ().GetTimeStep ())
{
}

void
SeqTsHeader::SetSeq (uint32_t seq)
{
  m_seq = seq;
}
uint32_t
SeqTsHeader::GetSeq (void) const
{
  return m_seq;
}

void
SeqTsHeader::SethqCnt_qCnt (uint32_t hqCnt_qCnt) 
{
	m_hqCnt_qCnt = hqCnt_qCnt;
}

uint32_t
SeqTsHeader::GethqCnt_qCnt (void) const
{
	return m_hqCnt_qCnt;
}

void
SeqTsHeader::SetPG (uint16_t pg)
{
	m_pg = pg;
}
uint16_t
SeqTsHeader::GetPG (void) const
{
	return m_pg;
}

void
SeqTsHeader::SetEndSeq (uint32_t endseq)
{
	m_endseq = endseq;
}

uint32_t
SeqTsHeader::GetEndSeq (void) const
{
	return m_endseq;
}

Time
SeqTsHeader::GetTs (void) const
{
  return TimeStep (m_ts);
}

uint64_t
SeqTsHeader::GetTsUint (void) const
{
  return m_ts;
}

void
SeqTsHeader::SetTs (uint32_t t)
{
  m_ts = uint64_t(t);
}

TypeId
SeqTsHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SeqTsHeader")
    .SetParent<Header> ()
    .AddConstructor<SeqTsHeader> ()
  ;
  return tid;
}
TypeId
SeqTsHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
SeqTsHeader::Print (std::ostream &os) const
{
  //os << "(seq=" << m_seq << " time=" << TimeStep (m_ts).GetSeconds () << ")";
	//os << m_seq << " " << TimeStep (m_ts).GetSeconds () << " " << m_pg;
	os << m_seq << " " << m_pg;
}
uint32_t
SeqTsHeader::GetSerializedSize (void) const
{
  return 22;
}

void
SeqTsHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_seq);
  i.WriteHtonU64 (m_ts);
  i.WriteHtonU16 (m_pg);
  i.WriteHtonU32 (m_endseq);
  i.WriteHtonU32 (m_hqCnt_qCnt);
}
uint32_t
SeqTsHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_seq = i.ReadNtohU32 ();
  m_ts =  i.ReadNtohU64 ();
  m_pg =  i.ReadNtohU16 ();
  m_endseq = i.ReadNtohU32 ();
  m_hqCnt_qCnt = i.ReadNtohU32 ();
  return GetSerializedSize ();
}

} // namespace ns3
