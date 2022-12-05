/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
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
#include "ns3/abort.h"
#include "ns3/log.h"
#include "custom-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CustomHeader");

NS_OBJECT_ENSURE_REGISTERED (CustomHeader);

CustomHeader::CustomHeader ()
  : brief(1), headerType(L3_Header | L4_Header), 
	// ppp header
	pppProto (0),
	// IPv4 header
    m_payloadSize (0),
    ipid (0),
    m_tos (0),
    m_ttl (0),
    l3Prot (0),
    ipv4Flags (0),
    m_fragmentOffset (0),
    m_checksum (0),
    m_headerSize(5*4)
{
}
CustomHeader::CustomHeader (uint32_t _headerType)
  : brief(1), headerType(_headerType), 
	// ppp header
	pppProto (0),
	// IPv4 header
    m_payloadSize (0),
    ipid (0),
    m_tos (0),
    m_ttl (0),
    l3Prot (0),
    ipv4Flags (0),
    m_fragmentOffset (0),
    m_checksum (0),
    m_headerSize(5*4)
{
}

TypeId 
CustomHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CustomHeader")
    .SetParent<Header> ()
    .SetGroupName ("Network")
    .AddConstructor<CustomHeader> ()
  ;
  return tid;
}
TypeId 
CustomHeader::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void CustomHeader::Print (std::ostream &os) const{
}
uint32_t CustomHeader::GetSerializedSize (void) const{
	uint32_t len = 0;
	if (headerType & L2_Header)
		len += 2;
	if (headerType & L3_Header)
		len += 5*4;
	if (headerType & L4_Header){
		if (l3Prot == 0x6) // TCP
			len += tcp.length * 4;
		else if (l3Prot == 0x11) // UDP
			len += 8 + 14;
		else if (l3Prot == 0xFC || l3Prot == 0xFD)
			len += 8;
		else if (l3Prot == 0xFF)
			len += 8;
		else if (l3Prot == 0xFE)
			len += 9;
	}
	return len;
}
void CustomHeader::Serialize (Buffer::Iterator start) const{
  Buffer::Iterator i = start;
  
  // ppp
  if (headerType & L2_Header){
	  i.WriteHtonU16(pppProto);
  }

  // IPv4
  if (headerType & L3_Header){
	  uint8_t verIhl = (4 << 4) | (5);
	  i.WriteU8 (verIhl);
	  i.WriteU8 (m_tos);
	  i.WriteHtonU16 (m_payloadSize + 5*4);
	  i.WriteHtonU16 (ipid);
	  uint32_t fragmentOffset = m_fragmentOffset / 8;
	  uint8_t flagsFrag = (fragmentOffset >> 8) & 0x1f;
	  if (ipv4Flags & DONT_FRAGMENT) 
		  flagsFrag |= (1<<6);
	  if (ipv4Flags & MORE_FRAGMENTS) 
		  flagsFrag |= (1<<5);
	  i.WriteU8 (flagsFrag);
	  uint8_t frag = fragmentOffset & 0xff;
	  i.WriteU8 (frag);
	  i.WriteU8 (m_ttl);
	  i.WriteU8 (l3Prot);
	  i.WriteHtonU16 (0);
	  i.WriteHtonU32 (sip);
	  i.WriteHtonU32 (dip);
  }

  // L4
  if (headerType & L4_Header){
	  if (l3Prot == 0x6){ // TCP
		  i.WriteHtonU16 (tcp.sport);
		  i.WriteHtonU16 (tcp.dport);
		  i.WriteHtonU32 (tcp.seq);
		  i.WriteHtonU32 (tcp.ack);
		  i.WriteHtonU16 (tcp.length << 12 | tcp.tcpFlags); //reserved bits are all zero
		  i.WriteHtonU16 (tcp.windowSize);
		  i.WriteHtonU16 (0);
		  i.WriteHtonU16 (tcp.urgentPointer);

		  uint32_t optionLen = (tcp.length - 5) * 4;
		  if (optionLen <= 32)
			  i.Write(tcp.optionBuf, optionLen);
	  }else if (l3Prot == 0x11){ // UDP
		  // udp header
		  i.WriteHtonU16 (udp.sport);
		  i.WriteHtonU16 (udp.dport);
		  i.WriteHtonU16 (udp.payload_size);
		  i.WriteHtonU16 (0);
		  // SeqTsHeader
		  i.WriteHtonU32 (udp.seq);
		  i.WriteHtonU64 (udp.ts);
		  i.WriteHtonU16 (udp.pg);
		  i.WriteHtonU32 (udp.endseq);
		  i.WriteHtonU32 (udp.hqCnt_qCnt);
	  }else if (l3Prot == 0xFF){ // CNP
		  i.WriteU8(cnp.qIndex);
		  i.WriteU16(cnp.fid);
		  i.WriteU8(cnp.ecnBits);
		  i.WriteU16(cnp.qfb);
		  i.WriteU16(cnp.total);
	  }else if (l3Prot == 0xFC || l3Prot == 0xFD){ // ACK or NACK
		  i.WriteU16(ack.pg);
		  i.WriteU32(ack.seq);
		  i.WriteU16(ack.port);
	  }else if (l3Prot == 0xFE){ // PFC
		  i.WriteU32 (pfc.time);
		  i.WriteU16 (pfc.num);
		  for (uint16_t x = 0; x  < pfc.num; x++) {
			  i.WriteU16 (pfc.pause_map[x]);
		  }
		  /*i.WriteU64 (pfc.pause_map_a);
		  i.WriteU64 (pfc.pause_map_b);
		  i.WriteU64 (pfc.pause_map_c);
		  i.WriteU64 (pfc.pause_map_d);
		  i.WriteU64 (pfc.pause_map_e);
		  i.WriteU64 (pfc.pause_map_f);
		  i.WriteU64 (pfc.pause_map_g);
		  i.WriteU64 (pfc.pause_map_h);
		  i.WriteU64 (pfc.pause_map_i);
		  i.WriteU64 (pfc.pause_map_j);
		  i.WriteU64 (pfc.pause_map_k);
		  i.WriteU64 (pfc.pause_map_l);
		  i.WriteU64 (pfc.pause_map_m);
		  i.WriteU64 (pfc.pause_map_n);
		  i.WriteU64 (pfc.pause_map_o);
		  i.WriteU64 (pfc.pause_map_p);*/
	  }
  }
}

uint32_t
CustomHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  // L2
  int l2Size = 0;
  if (headerType & L2_Header){
	  pppProto = i.ReadNtohU16();
	  l2Size = 2;
  }

  // L3
  int l3Size = 0;
  if (headerType & L3_Header){
	  i = start;
	  i.Next(l2Size);

	  uint8_t verIhl = i.ReadU8 ();
	  uint8_t ihl = verIhl & 0x0f; 
	  uint16_t headerSize = ihl * 4;
	  l3Size = headerSize;

	  if ((verIhl >> 4) != 4)
	  {
		  NS_LOG_WARN ("Trying to decode a non-IPv4 header, refusing to do it.");
		  return 0;
	  }

	  if (brief){
		  m_tos = i.ReadU8 ();
		  i.Next(2);
		  ipid = i.ReadNtohU16();
		  i.Next(3);
		  l3Prot = i.ReadU8();
		  i.Next(2);
		  sip = i.ReadNtohU32();
		  dip = i.ReadNtohU32();
	  } else {
		  m_tos = i.ReadU8 ();
		  uint16_t size = i.ReadNtohU16 ();
		  m_payloadSize = size - headerSize;
		  ipid = i.ReadNtohU16 ();
		  uint8_t flags = i.ReadU8 ();
		  ipv4Flags = 0;
		  if (flags & (1<<6)) 
			  ipv4Flags |= DONT_FRAGMENT;
		  if (flags & (1<<5)) 
			  ipv4Flags |= MORE_FRAGMENTS;
		  i.Prev ();
		  m_fragmentOffset = i.ReadU8 () & 0x1f;
		  m_fragmentOffset <<= 8;
		  m_fragmentOffset |= i.ReadU8 ();
		  m_fragmentOffset <<= 3;
		  m_ttl = i.ReadU8 ();
		  l3Prot = i.ReadU8 ();
		  m_checksum = i.ReadU16 ();
		  /* i.Next (2); // checksum */
		  sip = i.ReadNtohU32 ();
		  dip = i.ReadNtohU32 ();
		  m_headerSize = headerSize;
	  }
  }

  // TCP
  int l4Size = 0;
  if (headerType & L4_Header){
	  if (l3Prot == 0x6){ // TCP
		  i = start;
		  i.Next(l2Size + l3Size);
		  tcp.sport = i.ReadNtohU16 ();
		  tcp.dport = i.ReadNtohU16 ();
		  tcp.seq = i.ReadNtohU32 ();
		  tcp.ack = i.ReadNtohU32 ();
		  if (brief){
			  tcp.tcpFlags = i.ReadNtohU16() & 0x3f;
		  }else {
			  uint16_t field = i.ReadNtohU16 ();
			  tcp.tcpFlags = field & 0x3F;
			  tcp.length = field >> 12;
			  tcp.windowSize = i.ReadNtohU16 ();
			  i.Next (2);
			  tcp.urgentPointer = i.ReadNtohU16 ();

			  uint32_t optionLen = (tcp.length - 5) * 4;
			  if (optionLen > 32)
			  {
				  NS_LOG_ERROR ("TCP option length " << optionLen << " > 32; options discarded");
				  return 20;
			  }
			  i.Read(tcp.optionBuf, optionLen);
		  }
		  l4Size = tcp.length * 4;
	  }else if (l3Prot == 0x11){ // UDP + SeqTsHeader
		  i = start;
		  i.Next(l2Size + l3Size);
		  // udp header
		  udp.sport = i.ReadNtohU16 ();
		  udp.dport = i.ReadNtohU16 ();
		  if (brief){
			  i.Next(4);
		  }else{
			  udp.payload_size = i.ReadNtohU16();
			  i.Next(2);
		  }

		  // SeqTsHeader
		  udp.seq = i.ReadNtohU32 ();
		  udp.ts =  i.ReadNtohU64 ();
		  udp.pg =  i.ReadNtohU16 ();
		  udp.endseq = i.ReadNtohU32 ();
		  udp.hqCnt_qCnt = i.ReadNtohU32 ();

		  l4Size = 8 + 22;
	  }else if (l3Prot == 0xFF){
		  cnp.qIndex = i.ReadU8();
		  cnp.fid = i.ReadU16();
		  cnp.ecnBits = i.ReadU8();
		  cnp.qfb = i.ReadU16();
		  cnp.total = i.ReadU16();
		  l4Size = 8;
	  }else if (l3Prot == 0xFC || l3Prot == 0xFD){ // ACK or NACK
		  ack.pg = i.ReadU16();
		  ack.seq = i.ReadU32();
		  ack.port = i.ReadU16();
		  l4Size = 8;
	  }else if (l3Prot == 0xFE){ // PFC
		  pfc.time = i.ReadU32 ();
		  pfc.num = i.ReadU16 ();
		  //pfc.pause_map.clear();
		  for (uint16_t x = 0; x < pfc.num; x++) {
			  pfc.pause_map[x] = i.ReadNtohU16 ();
		  }
		  /*pfc.pause_map_a = i.ReadU64 ();
		  pfc.pause_map_b = i.ReadU64 ();
		  pfc.pause_map_c = i.ReadU64 ();
		  pfc.pause_map_d = i.ReadU64 ();
		  pfc.pause_map_e = i.ReadU64 ();
		  pfc.pause_map_f = i.ReadU64 ();
		  pfc.pause_map_g = i.ReadU64 ();
		  pfc.pause_map_h = i.ReadU64 ();
		  pfc.pause_map_i = i.ReadU64 ();
		  pfc.pause_map_j = i.ReadU64 ();
		  pfc.pause_map_k = i.ReadU64 ();
		  pfc.pause_map_l = i.ReadU64 ();
		  pfc.pause_map_m = i.ReadU64 ();
		  pfc.pause_map_n = i.ReadU64 ();
		  pfc.pause_map_o = i.ReadU64 ();
		  pfc.pause_map_p = i.ReadU64 ();*/
		  l4Size = 6 + 2 * pfc.num;
	  }
  }

  return l2Size + l3Size + l4Size;
#if 0

  Buffer::Iterator i = start;
  
  int l2Size = 0;
  if (hasPppHdr){
	  pppProto = i.ReadNtohU16();
	  l2Size = 2;
  }

  // Ipv4
  uint16_t ipv4HeaderSize = 0;
  if (hasIpv4Hdr){
	  uint8_t verIhl = i.ReadU8 ();
	  uint8_t ihl = verIhl & 0x0f; 
	  ipv4HeaderSize = ihl * 4;

	  i.Next(3);
	  ipid = i.ReadNtohU16();
	  i.Next(6);
	  sip = i.ReadNtohU32();
	  dip = i.ReadNtohU32();

	  i = start;
	  i.Next(l2Size + ipv4HeaderSize);
  }

  // tcp
  sport = i.ReadNtohU16();
  dport = i.ReadNtohU16();
  seq = i.ReadNtohU32();
  ack = i.ReadNtohU32();
  flags = i.ReadNtohU16() & 0x3f;

  return l2Size + ipv4HeaderSize + 14;
#endif
}

} // namespace ns3

