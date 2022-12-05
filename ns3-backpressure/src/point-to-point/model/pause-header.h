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

#ifndef PAUSE_HEADER_H
#define PAUSE_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/buffer.h"
#include <vector>

namespace ns3 {

/**
 * \ingroup Pause
 * \brief Header for the Congestion Notification Message
 *
 * This class has two fields: The five-tuple flow id and the quantized
 * congestion level. This can be serialized to or deserialzed from a byte
 * buffer.
 */

class PauseHeader : public Header
{
public:
  PauseHeader (uint32_t time, uint16_t num, uint16_t pause_map[]);//,uint64_t pause_map_b);
  PauseHeader ();
  virtual ~PauseHeader ();

//Setters
  /**
   * \param time The pause time in microseconds
   */
  void SetTime (uint32_t time);
  //void SetPMap (uint16_t m_num, uint16_t pause_map[]);
  //void SetPMapB (uint64_t pause_map_b);

//Getters
  /**
   * \return The pause time in microseconds
   */
  uint32_t GetTime () const;
  uint16_t GetNum () const;
  void GetPMap (uint16_t pause_map[]);
  //uint64_t GetPMapB () const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint32_t m_time;
  uint16_t m_num;
  uint16_t m_pause_map[100];
  /*uint64_t m_pause_map_a;
  uint64_t m_pause_map_b;

  uint64_t m_pause_map_c;
  uint64_t m_pause_map_d;
  uint64_t m_pause_map_e;
  uint64_t m_pause_map_f;
  uint64_t m_pause_map_g;
  uint64_t m_pause_map_h;
  uint64_t m_pause_map_i;
  uint64_t m_pause_map_j;
  uint64_t m_pause_map_k;
  uint64_t m_pause_map_l;
  uint64_t m_pause_map_m;
  uint64_t m_pause_map_n;
  uint64_t m_pause_map_o;
  uint64_t m_pause_map_p;*/

};

}; // namespace ns3

#endif /* PAUSE_HEADER */
