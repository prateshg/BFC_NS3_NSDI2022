/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#ifndef BROADCOM_EGRESS_H
#define BROADCOM_EGRESS_H

#include <queue>
#include "ns3/packet.h"
#include "queue.h"
#include "drop-tail-queue.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/event-id.h"
#include <set>

namespace ns3 {

	class TraceContainer;

	class BEgressQueue : public Queue {
	public:
		static TypeId GetTypeId(void);
		static const unsigned fCnt = 2048; //max number of queues, 128 for NICs
		static const unsigned qCnt = 36; //max number of queues, 8 for switches
		BEgressQueue();
		virtual ~BEgressQueue();
		bool Enqueue(Ptr<Packet> p, uint32_t qIndex);
		Ptr<Packet> Dequeue(bool paused[]);
		Ptr<Packet> DequeueRR(uint64_t paused[]);
		Ptr<Packet> DequeueNIC(bool paused[]);//QCN disable NIC
		Ptr<Packet> DequeueQCN(uint64_t paused[],uint32_t m_findex_qindex_map[], uint32_t curr_seq[], uint32_t m_acked[], uint32_t window[], bool enable_window);//QCN enable NIC
		uint32_t GetNBytes(uint32_t qIndex) const;
		uint32_t GetNBytesTotal() const;
		uint32_t GetLastQueue();
		uint32_t m_fcount;
		void RecoverQueue(Ptr<DropTailQueue> buffer, uint32_t i);

		TracedCallback<Ptr<const Packet>, uint32_t> m_traceBeqEnqueue;
		TracedCallback<Ptr<const Packet>, uint32_t> m_traceBeqDequeue;
		std::vector<Ptr<Queue> > m_queues;
		uint64_t active_queues_arr[(fCnt - 1)/64 + 1];
		std::set<uint32_t> active_queues;


	private:
		bool DoEnqueue(Ptr<Packet> p, uint32_t qIndex);
		Ptr<Packet> DoDequeue(bool paused[]);
		Ptr<Packet> DoDequeueNIC(bool paused[]);
		Ptr<Packet> DoDequeueRR(uint64_t paused[]);
		Ptr<Packet> DoDequeueQCN(uint64_t paused[], uint32_t m_findex_qindex_map[], uint32_t curr_seq[], uint32_t m_acked[], uint32_t window[], bool enable_window);
		//for compatibility
		virtual bool DoEnqueue(Ptr<Packet> p);
		virtual Ptr<Packet> DoDequeue(void);
		virtual Ptr<const Packet> DoPeek(void) const;
		std::queue<Ptr<Packet> > m_packets;
		uint32_t m_maxPackets;
		double m_maxBytes; //total bytes limit
		uint32_t m_qmincell; //guaranteed see page 126
		uint32_t m_queuelimit; //limit for each queue
		uint32_t m_shareused; //used bytes by sharing
		uint32_t m_bytesInQueue[fCnt];
		uint32_t m_bytesInQueueTotal;
		uint32_t m_rrlast;
		uint32_t m_qlast;
		QueueMode m_mode;
		//std::vector<Ptr<Queue> > m_queues; // uc queues
		//For strict priority
		Time m_bwsatisfied[qCnt];
		DataRate m_minBW[qCnt];

		uint64_t power_of_two[64];
		uint64_t power_of_two_agg[64];
	};

} // namespace ns3

#endif /* DROPTAIL_H */
