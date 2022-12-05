/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- *
/*
* Copyright (c) 2006 Georgia Tech Research Corporation, INRIA
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
* Author: Yibo Zhu <yibzh@microsoft.com>
*/
#ifndef QBB_NET_DEVICE_H
#define QBB_NET_DEVICE_H

#include "ns3/point-to-point-net-device.h"
#include "ns3/broadcom-node.h"
#include "ns3/qbb-channel.h"
//#include "ns3/fivetuple.h"
#include "ns3/event-id.h"
#include "ns3/broadcom-egress-queue.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/seq-ts-header.h"
#include "ns3/udp-header.h"
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <queue>

namespace ns3 {

/**
 * \class QbbNetDevice
 * \brief A Device for a IEEE 802.1Qbb Network Link.
 */
class QbbNetDevice : public PointToPointNetDevice 
{
public:
  static const uint32_t qCnt = 36;	// Number of queues/priorities used
  static const uint32_t pCnt = 64;	// Number of ports used
  static const uint32_t fCnt = 2048; // Max number of flows on a NIC, for TX and RX respectively. TX+RX=fCnt*2
  static const uint32_t hqCnt = pCnt*qCnt*200;
  uint32_t GethqCnt(Ptr<Packet> p, bool eq, uint32_t outDev);
  uint32_t Assign_hqCnt_qCnt(uint32_t p_hqCnt);
  void RemoveAssignedQueue(uint32_t p_hqCnt);
  static const uint32_t maxHop = 1; // Max hop count in the network. should not exceed 16 
  static const uint32_t bloom_filter_length = 1024;
  uint16_t bloom_filter_counter[bloom_filter_length];
  uint16_t bloom_filter[bloom_filter_length/16];
  uint16_t received_bloom_filter[bloom_filter_length/16];
  std::queue<std::pair<uint32_t, uint8_t> > paused_empty_list[qCnt];
  double timestamp_paused[qCnt];
  std::queue<uint32_t> egress_physicalQ_head_virtualQ_Index[fCnt];
  static TypeId GetTypeId (void);
  void Update_Paused_PhysicalQ(uint32_t physicalQ);
  QbbNetDevice ();
  virtual ~QbbNetDevice ();
  /**
   * Receive a packet from a connected PointToPointChannel.
   *
   * This is to intercept the same call from the PointToPointNetDevice
   * so that the pause messages are honoured without letting
   * PointToPointNetDevice::Receive(p) know
   *
   * @see PointToPointNetDevice
   * @param p Ptr to the received packet.
   */
  virtual void Receive (Ptr<Packet> p);
  bool EnqueueQbb(Ptr<Packet> p, uint32_t qIndex);

  /**
   * Send a packet to the channel by putting it to the queue
   * of the corresponding priority class
   *
   * @param packet Ptr to the packet to send
   * @param dest Unused
   * @param protocolNumber Protocol used in packet
   */
  virtual bool Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  /**
   * Get the size of Tx buffer available in the device
   *
   * @return buffer available in bytes
   */
  //virtual uint32_t GetTxAvailable(unsigned) const;

  /**
   * TracedCallback hooks
   */
  void ConnectWithoutContext(const CallbackBase& callback);
  void DisconnectWithoutContext(const CallbackBase& callback);

  virtual int32_t PrintStatus(std::ostream& os);

  bool Attach(Ptr<QbbChannel> ch);
  
  uint32_t GetPriority(Ptr<Packet> p);
  uint32_t GetSeq(Ptr<Packet> p);
  SeqTsHeader GetSth(Ptr<Packet> p);
  Ipv4Address GetSource(Ptr<Packet> p);
  uint32_t GetPort(Ptr<Packet> p);

   void SetBroadcomParams(
	    uint32_t pausetime,
		double qcn_interval,
		double qcn_resume_interval,
		double g,
		DataRate minRate,
		DataRate rai,
		uint32_t m_fastrecover_times
	   );

   virtual Ptr<Channel> GetChannel (void) const;

   virtual uint32_t GetUsedBuffer(uint32_t port, uint32_t qIndex);

   void SetQueue (Ptr<BEgressQueue> q);
   Ptr<BEgressQueue> GetQueue ();
  virtual void FlowFinished(uint32_t port, uint32_t priority, uint32_t lastseq);
  virtual void SendPauseFrame(std::vector<int> pClasses[]);//uint64_t pause_map_a, uint64_t pause_map_b);

  virtual void UpdatePauseQueue(uint32_t qIndex, uint32_t p_hqCnt);
  virtual void UpdateResumeQueue(uint32_t qIndex, uint32_t p_hqCnt);
  uint32_t GethqCntqCnt(Ptr<Packet> p);

  bool is_queue_paused(uint32_t qIndex);

    bool m_enable_window;
    uint32_t curr_seq[fCnt];
  uint32_t m_acked[fCnt];
  uint32_t window[fCnt];

  uint32_t fixed_window_size;
  //virtual uint32_t m_usedQueues;
protected:

	//Ptr<Node> m_node;

  bool TransmitStart (Ptr<Packet> p);
  
  Address GetRemote (void) const;
  
  void PointToPointReceive (Ptr<Packet> packet);


  virtual void DoDispose(void);

  /// Reset the channel into READY state and try transmit again
  virtual void TransmitComplete(void);

  /// Look for an available packet and send it using TransmitStart(p)
  virtual void DequeueAndTransmit(void);

  /// Resume a paused queue and call DequeueAndTransmit()
  virtual void Resume(unsigned qIndex);

  /// Wrapper to Resume() as an schedulable function
  void PauseFinish(unsigned qIndex);



  /// Tell if an address is local to this node
  bool IsLocal(const Ipv4Address& addr) const;

  /**
   * The queues for each priority class.
   * @see class Queue
   * @see class InfiniteQueue
   */
  Ptr<BEgressQueue> m_queue;

  TracedCallback<Ptr<NetDevice>, uint32_t> m_sendCb;	//< Callback chain for notifying Tx buffer available

  Ptr<QbbChannel> m_channel;
  
  //pfc
  //bool m_qbbEnabled;	//< PFC behaviour enabledR
  //bool m_dynamicth;R
  //uint32_t m_threshold;	//< Threshold for generating Pause
  uint32_t m_pausetime;	//< Time for each Pause
  //uint32_t m_buffersize;//< Total size of Tx buffer
  //uint32_t m_bufferUsage;	//< Occupancy at the buffer
  bool m_paused[qCnt];	//< Whether a queue paused
  uint64_t paused_map[(fCnt - 1)/64 + 1];
  double m_lastResume[qCnt];
  EventId m_resumeEvt[qCnt];  //< Keeping the next resume event (PFC)
  EventId m_recheckEvt[pCnt][qCnt]; //< Keeping the next recheck queue full event (PFC)

  //qcn

  //cp

  
  /* State variable for rate-limited queues */

  uint32_t m_lastseq[fCnt];
  bool m_available_queue[fCnt];

  //bool m_extraFastRecovery[fCnt][maxHop]; //false means this is the first time receive CNP
  

  //qcn

  struct ECNAccount{
	  Ipv4Address source;
	  uint32_t qIndex;
	  uint32_t port;
  };

  std::vector<ECNAccount> *m_ecn_source;
   uint32_t m_findex_qindex_map[fCnt];


  int ReceiverCheckSeq(uint32_t seq, uint32_t key, uint32_t fseq, uint32_t port, Ipv4Address src, uint32_t qIndex, uint32_t size);
  void Retransmit(uint32_t findex);
  double m_nack_interval;
  Ptr<DropTailQueue> m_sendingBuffer[fCnt];
  uint32_t m_chunk;
  uint32_t m_ack_interval;
  uint32_t ReceiverNextExpectedSeq[fCnt];
  uint32_t qbbid;
  Time m_nackTimer[fCnt];
  uint32_t m_lastNACK[fCnt];
  int32_t m_milestone_rx[fCnt];
  bool m_backto0;
  bool m_testRead;
  int32_t m_milestone_tx[fCnt];
  EventId m_retransmit[fCnt];
  bool m_available_receiving[fCnt];

  bool UpdatePauseState(uint32_t inDev, uint32_t qIndex, uint32_t p_hqCnt);
  void GetCentralArrayIndex (uint32_t p_hqCnt);

  std::unordered_map< uint32_t, std::unordered_map<uint32_t,  uint32_t> > queue_index_map;
  std::unordered_map< uint32_t, std::unordered_map<uint32_t,  std::unordered_map<uint32_t,  uint32_t> > > receiver_index_map;
  std::unordered_map< uint32_t, Time> starting_times;
  //std::unordered_map< uint32_t, std::unordered_map<uint32_t,  uint32_t> > receive_index_map;
  int x;
  bool egress_paused_virtualQ_ingress[pCnt][hqCnt];

  uint64_t power_of_two[64];
  std::queue<uint32_t> available_queue_vector;

  std::queue<uint32_t> available_sending_queues;
  std::unordered_set<uint32_t> available_receiving_queues;
  EventId m_nextSend;
  

};

} // namespace ns3

#endif // QBB_NET_DEVICE_H

