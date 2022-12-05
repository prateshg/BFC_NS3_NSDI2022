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

#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
#include <stdio.h>
#include "ns3/qbb-net-device.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/data-rate.h"
#include "ns3/object-vector.h"
#include "ns3/pause-header.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/assert.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/qbb-channel.h"
#include "ns3/random-variable.h"
#include "ns3/flow-id-tag.h"
#include "ns3/qbb-header.h"
#include "ns3/error-model.h"
#include "ns3/cn-header.h"
#include "ns3/ppp-header.h"
#include "ns3/udp-header.h"
#include "ns3/seq-ts-header.h"
#include "ns3/pointer.h"
#include <bits/stdc++.h>

#include <iostream>

NS_LOG_COMPONENT_DEFINE("QbbNetDevice");

namespace ns3 {

	NS_OBJECT_ENSURE_REGISTERED(QbbNetDevice);

	TypeId
		QbbNetDevice::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::QbbNetDevice")
			.SetParent<PointToPointNetDevice>()
			.AddConstructor<QbbNetDevice>()
			.AddAttribute("PauseTime",
				"Number of microseconds to pause upon congestion",
				UintegerValue(5),
				MakeUintegerAccessor(&QbbNetDevice::m_pausetime),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("NACK Generation Interval",
				"The NACK Generation interval",
				DoubleValue(500.0),
				MakeDoubleAccessor(&QbbNetDevice::m_nack_interval),
				MakeDoubleChecker<double>())
			.AddAttribute("L2BackToZero",
				"Layer 2 go back to zero transmission.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_backto0),
				MakeBooleanChecker())
			.AddAttribute("L2TestRead",
				"Layer 2 test read go back 0 but NACK from n.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_testRead),
				MakeBooleanChecker())
			.AddAttribute("L2ChunkSize",
				"Layer 2 chunk size. Disable chunk mode if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&QbbNetDevice::m_chunk),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("EnableWindow",
				"Window is enabled",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_enable_window),
				MakeBooleanChecker())
			.AddAttribute("FixedWindowSize",
				"Fixed Window Size, in case enabled",
				UintegerValue(15),
				MakeUintegerAccessor(&QbbNetDevice::fixed_window_size),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("L2AckInterval",
				"Layer 2 Ack intervals. Disable ack if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&QbbNetDevice::m_ack_interval),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute ("TxBeQueue", 
					"A queue to use as the transmit queue in the device.",
					PointerValue (),
					MakePointerAccessor (&QbbNetDevice::m_queue),
					MakePointerChecker<Queue> ())
			;

		return tid;
	}

uint32_t id = 1;
uint32_t n_int = 10;
	QbbNetDevice::QbbNetDevice()
	{
		NS_LOG_FUNCTION(this);
		m_ecn_source = new std::vector<ECNAccount>;
		for (uint32_t i = 3; i < qCnt-2; i++) {
			available_queue_vector.push(i);
		}
		for (uint32_t i = 0; i < qCnt; i++)
		{
			m_paused[i] = false;
			m_lastResume[i] = 0.0;
		}
		//m_usedQueues = 0;
		for(int i=0;i<((fCnt-1)/64)+1;i++)
		{
			paused_map[i]=0;
		}
		for(int i=0;i<64;i++)
		{
			power_of_two[i] = ((uint64_t)1<<i);
		}
		//std::cout<<"m_bps = "<<m_bps<<"\n";
		srand(id);
		std::vector<uint32_t> tempv;
		for (uint32_t i = 1; i < fCnt; i++)
		{
			tempv.push_back(i);
		}
		//random_shuffle(tempv.begin(), tempv.end());
		shuffle(tempv.begin(), tempv.end(), std::default_random_engine(rand()));
		for (uint32_t i = 0; i < fCnt; i++)
		{
			NS_ASSERT(egress_physicalQ_head_virtualQ_Index[i].size() == 0);
			m_acked[i] = 0;
			curr_seq[i] = 0;
			window[i] = fixed_window_size;

			m_sendingBuffer[i] = CreateObject<DropTailQueue>();
            m_lastseq[i] = 0;
            m_available_receiving[i] = true;
            m_available_queue[i] = true;
			m_findex_qindex_map[i] = 0;
			if(i>=1)
			{
				available_sending_queues.push(tempv[i-1]);
				//std::cout<<i<<":"<<tempv[i-1]<<" ";
				available_receiving_queues.insert(i);
			}
		}
		srand(1);
		m_ecn_source->resize(fCnt);
		qbbid = id;
		id++;
		//x=0;
		m_usedQueues = 1;
		//std::cout<<" qbbid "<<qbbid<<" IP "<<m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal()<<"\n";
	}


	QbbNetDevice::~QbbNetDevice()
	{
		NS_LOG_FUNCTION(this);
	}

	void
		QbbNetDevice::DoDispose()
	{
		std::cerr<<"Entered do dispose\n";
		NS_LOG_FUNCTION(this);
		// Cancel all the Qbb events
		for (uint32_t i = 0; i < qCnt; i++)
		{
			Simulator::Cancel(m_resumeEvt[i]);
		}


		for (uint32_t i = 0; i < pCnt; i++)
			for (uint32_t j = 0; j < qCnt; j++)
				Simulator::Cancel(m_recheckEvt[i][j]);
		PointToPointNetDevice::DoDispose();
	}

	void
		QbbNetDevice::TransmitComplete(void)
	{
		//NS_LOG_FUNCTION(this);
		NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
		m_txMachineState = READY;
		NS_ASSERT_MSG(m_currentPkt != 0, "QbbNetDevice::TransmitComplete(): m_currentPkt zero");
		//m_phyTxEndTrace(m_currentPkt);
		m_currentPkt = 0;
		DequeueAndTransmit();
	}

	void
		QbbNetDevice::DequeueAndTransmit(void)
	{

		//NS_LOG_FUNCTION(this);
		if (m_txMachineState == BUSY) return;	// Quit if channel busy
		Ptr<Packet> p;
		if (m_node->GetNodeType() == 0) //QCN disable NIC
		{
			p = m_queue->DequeueQCN(paused_map, m_findex_qindex_map, curr_seq, m_acked, window, m_enable_window);
		}
		else   //switch, doesn't care about qcn, just send
		{
			p = m_queue->DequeueRR(paused_map);		//this is round-robin
		}
		if (p != 0)
		{
			// m_snifferTrace(p);
			// m_promiscSnifferTrace(p);
			Ipv4Header h;
			Ptr<Packet> packet = p->Copy();
			uint16_t protocol = 0;
			ProcessHeader(packet, protocol);
			packet->RemoveHeader(h);
			FlowIdTag t;
			if (m_node->GetNodeType() == 0) //I am a NIC, do QCN
			{
				uint32_t fIndex = m_queue->GetLastQueue();
					if (m_enable_window && fIndex!=0 && !m_queue->m_queues[fIndex]->IsEmpty())
					{
						curr_seq[fIndex] = GetSeq(m_queue->m_queues[fIndex]->Peek()->Copy());
					}
					
				p->RemovePacketTag(t);
				TransmitStart(p);
			}
			else //I am a switch, do ECN if this is not a pause
			{
				if (m_queue->GetLastQueue() == qCnt - 1)//this is a pause or cnp, send it immediately!
				{
					//if(m_queue->GetNBytes(m_queue->GetLastQueue())==0)
					//{
						//m_usedQueues -= 1;
						//NS_ASSERT(m_usedQueues>=0);
					//}

				    p->RemovePacketTag(t);
					TransmitStart(p);
				}
				else
				{
					if(m_queue->GetNBytes(m_queue->GetLastQueue())==0)
					{
						NS_ASSERT(egress_physicalQ_head_virtualQ_Index[m_queue->GetLastQueue()].size() == 1);
					}
					//switch ECN
					p->PeekPacketTag(t);
					uint32_t inDev = t.GetFlowId();
				        SeqTsHeader sth1 = GetSth(p->Copy());
					uint32_t seq = sth1.GetSeq();
					uint32_t EndSeq = sth1.GetEndSeq(); 
					NS_ASSERT(inDev < pCnt);
					//CHECK??
					uint32_t prev_assigned_queue = GethqCntqCnt(p);
					uint32_t p_hqCnt = GethqCnt(p, true, m_ifIndex);//inDev);
					bool regPacket = false;

					if (m_queue->GetLastQueue() != 2 && m_queue->GetLastQueue() != qCnt -2) {
						GetCentralArrayIndex (p_hqCnt); 
						regPacket = true;
						NS_ASSERT(m_node->m_broadcom->central_entry_array[p_hqCnt].size > 0);
						m_node->m_broadcom->central_entry_array[p_hqCnt].size -= 1;
					}
					m_node->m_broadcom->RemoveFromIngressAdmission(inDev, m_queue->GetLastQueue(), p->GetSize(), m_ifIndex);//CHECK??--Earlier remove isend_pause_map_help_count_hqCnt[p_hqCnt]ngress was done later- while dequeue and transmit was happening- it can be in either ingress or egress- not both
					m_node->m_broadcom->RemoveFromEgressAdmission(m_ifIndex, m_queue->GetLastQueue(), p->GetSize());
					NS_ASSERT(m_queue->GetLastQueue() < fCnt);
					uint32_t head_q = egress_physicalQ_head_virtualQ_Index[m_queue->GetLastQueue()].front();
					egress_physicalQ_head_virtualQ_Index[m_queue->GetLastQueue()].pop();
					Update_Paused_PhysicalQ(m_queue->GetLastQueue());

					if (regPacket) {
						if (head_q >= 100000) {
							bool overload = UpdatePauseState(inDev, m_queue->GetLastQueue(), prev_assigned_queue);
							Ptr<Ipv4> m_ipv4 = m_node->GetObject<Ipv4>();
							Ptr<NetDevice> device = m_ipv4->GetNetDevice(inDev);
							device->UpdateResumeQueue (m_queue->GetLastQueue(), prev_assigned_queue);
						}
					} else if (m_queue->GetLastQueue() == 2) {
						NS_ASSERT(false);
					}
					PppHeader ppp;
  					p->RemoveHeader (ppp);
					Ipv4Header ipv4h;
					p->RemoveHeader(ipv4h);
					UdpHeader udph;
                			p->RemoveHeader(udph);
                			SeqTsHeader sth;
                			p->RemoveHeader(sth);
                			sth.SethqCnt_qCnt(m_queue->GetLastQueue());
                			p->AddHeader(sth);
                			p->AddHeader(udph);
					p->AddHeader(ipv4h);
					p->AddHeader(ppp);
					p->RemovePacketTag(t);
					TransmitStart(p);
				}
			}
			return;
		}
		else //No queue can deliver any packet
		{
			//NS_LOG_INFO("PAUSE prohibits send at node " << m_node->GetId());
			Simulator::Cancel(m_nextSend);
			m_nextSend = Simulator::Schedule(MicroSeconds(1), &QbbNetDevice::DequeueAndTransmit, this);
		}
		return;
	}

	void
		QbbNetDevice::Retransmit(uint32_t findex)
	{
		NS_ASSERT(false);
		std::cout << "Resending the message of flow " << findex << ".\n";
		fflush(stdout);
		m_queue->RecoverQueue(m_sendingBuffer[findex], findex);
		if(m_enable_window && m_queue->GetNBytes(findex)!=0)
		{
			curr_seq[findex] = GetSeq(m_queue->m_queues[findex]->Peek()->Copy());
		}
		DequeueAndTransmit();
	}


	void
		QbbNetDevice::Resume(unsigned qIndex)
	{
		NS_LOG_FUNCTION(this << qIndex);
		NS_ASSERT_MSG(m_paused[qIndex], "Must be PAUSEd");
		//std::cout<<"Resumed "<<qIndex<<" at "<<qbbid<<" Time "<<Simulator::Now()<<"\n";
		m_paused[qIndex] = false;
		m_lastResume[qIndex] = Simulator::Now().GetSeconds();
		NS_LOG_INFO("Node " << m_node->GetId() << " dev " << m_ifIndex << " queue " << qIndex <<
			" resumed at " << Simulator::Now().GetSeconds());
		DequeueAndTransmit();
	}

	void
		QbbNetDevice::PauseFinish(unsigned qIndex)
	{
		Resume(qIndex);
	}


	void
		QbbNetDevice::Receive(Ptr<Packet> packet)
	{
		//NS_LOG_FUNCTION(this << packet);

		if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
		{
			// 
			// If we have an error model and it indicates that it is time to lose a
			// corrupted packet, don't forward this packet up, let it go.
			//
			m_phyRxDropTrace(packet);
			return;
		}

		uint16_t protocol = 0;

		Ptr<Packet> p = packet->Copy();
		ProcessHeader(p, protocol);

		Ipv4Header ipv4h;
		p->RemoveHeader(ipv4h);

		if ((ipv4h.GetProtocol() != 0xFF && ipv4h.GetProtocol() != 0xFD && ipv4h.GetProtocol() != 0xFC) || m_node->GetNodeType() > 0)
		{ //This is not QCN feedback, not NACK, or I am a switch so I don't care
			if (ipv4h.GetProtocol() != 0xFE) //not PFC
			{
				packet->AddPacketTag(FlowIdTag(m_ifIndex));
				if (m_node->GetNodeType() == 0) //NIC
				{
					if (ipv4h.GetProtocol() == 17)	//look at udp only
					{

						UdpHeader udph;
						p->RemoveHeader(udph);
						SeqTsHeader sth;
						p->PeekHeader(sth);

						p->AddHeader(udph);

						bool found = false;
						uint32_t i, key = 0;
						//uint32_t port = udph.GetSourcePort();
						uint32_t size = (p->Copy())->GetSize();
						uint32_t source_int = (ipv4h.GetSource()).Get();
						uint32_t priority_grp = GetPriority(p->Copy());
						uint32_t prt = udph.GetSourcePort();

						if(receiver_index_map.find(source_int)==receiver_index_map.end()
							|| receiver_index_map[source_int].find(prt)== receiver_index_map[source_int].end()
							|| receiver_index_map[source_int][prt].find(priority_grp)==receiver_index_map[source_int][prt].end())
						{
							if(!available_receiving_queues.empty())
							{
									i = *(available_receiving_queues.begin());
									found = true;
									key = i;
									ECNAccount tmp;
									tmp.qIndex = priority_grp;
									tmp.source = ipv4h.GetSource();
									tmp.port = prt;
									NS_ASSERT(key < fCnt);
									ReceiverNextExpectedSeq[key] = 0;
									m_nackTimer[key] = Simulator::Now();
									m_milestone_rx[key] = m_ack_interval;
									m_lastNACK[key] = -1;
									(*m_ecn_source)[key] = tmp;
									available_receiving_queues.erase(i);
									m_available_receiving[key] = false;
									receiver_index_map[source_int][prt][priority_grp]= i;
							}
							else{
								std::cerr<<"Out of receiver queues\n";
								NS_ASSERT(false);
							}
						}
						else{
							i = receiver_index_map[source_int][prt][priority_grp];
							NS_ASSERT(!m_available_receiving[i]);
							found = true;
								key = i;
						}
						if(!found)	
						{
							std::cerr<<"Out of available queues at receiver end\n";	
						}
                        NS_ASSERT(i<fCnt);

						int x = ReceiverCheckSeq(sth.GetSeq(), key, uint32_t(sth.GetEndSeq()), prt, ipv4h.GetSource(), priority_grp, size);//sth.GetFSeq());
						if (x == 2) //generate NACK
						{
							Ptr<Packet> newp = Create<Packet>(0);
							qbbHeader seqh;
							seqh.SetSeq(ReceiverNextExpectedSeq[key]);
							seqh.SetPG(priority_grp);
							seqh.SetPort(prt);
							newp->AddHeader(seqh);
							Ipv4Header head;	// Prepare IPv4 header
							head.SetDestination(ipv4h.GetSource());
							Ipv4Address myAddr = m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal();
							head.SetSource(myAddr);
							head.SetProtocol(0xFD); //nack=0xFD
							head.SetTtl(64);
							head.SetPayloadSize(newp->GetSize());
							head.SetIdentification(UniformVariable(0, 65536).GetValue());
							newp->AddHeader(head);
							uint32_t protocolNumber = 2048;
							AddHeader(newp, protocolNumber);	// Attach PPP header
								EnqueueQbb(newp, 0);
							DequeueAndTransmit();
						}
						else if (x == 1) //generate ACK
						{
							Ptr<Packet> newp = Create<Packet>(0);
							qbbHeader seqh;
							seqh.SetSeq(ReceiverNextExpectedSeq[key]);
							seqh.SetPG(priority_grp);
							seqh.SetPort(prt);
							newp->AddHeader(seqh);
							Ipv4Header head;	// Prepare IPv4 header
							head.SetDestination(ipv4h.GetSource());
							Ipv4Address myAddr = m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal();
							head.SetSource(myAddr);
							head.SetProtocol(0xFC); //ack=0xFC
							head.SetTtl(64);
							head.SetPayloadSize(newp->GetSize());
							head.SetIdentification(UniformVariable(0, 65536).GetValue());
							newp->AddHeader(head);
							uint32_t protocolNumber = 2048;
							AddHeader(newp, protocolNumber);	// Attach PPP header
								EnqueueQbb(newp, 0);
							DequeueAndTransmit();
						}
					}
				}
				PointToPointReceive(packet);
			}
			else // If this is a Pause, stop the corresponding queue
			{
				PauseHeader pauseh;
				p->RemoveHeader(pauseh);


		uint16_t num = pauseh.GetNum();
		uint16_t pause_diff[num];
	        pauseh.GetPMap (pause_diff);
		for (uint16_t i = 0; i < num ; i++) {
			received_bloom_filter[i%((bloom_filter_length)/16)] = pause_diff[i];
		}
		if (m_node->GetNodeType() == 0) {
			for (int i = 0; i < fCnt; i++) {
				Update_Paused_PhysicalQ(i);
			}
		} else {
			for (int i = 0; i < qCnt; i++) {
				Update_Paused_PhysicalQ(i);
			}
		}

                DequeueAndTransmit();

            }
		}

		else if (ipv4h.GetProtocol() == 0xFD)//NACK on NIC
		{
			qbbHeader qbbh;
			p->Copy()->RemoveHeader(qbbh);

			int qIndex = qbbh.GetPG();
			int seq = qbbh.GetSeq();
			int port = qbbh.GetPort();
			int i;
			if (queue_index_map.find(port) == queue_index_map.end() || queue_index_map[port].find(qIndex) == queue_index_map[port].end())
			{
				std::cout << "ERROR: NACK NIC cannot find the flow\n";
				return;
			}
			else{
				i = queue_index_map[port][qIndex];
			}

			if (!m_sendingBuffer[i]->IsEmpty())
			{
				uint32_t buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
				if (!m_backto0)
				{
					if (buffer_seq > seq)
					{
						std::cout << "ERROR: Sendingbuffer miss!\n";
					}
					while (seq > buffer_seq)
					{
						m_sendingBuffer[i]->Dequeue();
						if (m_sendingBuffer[i]->IsEmpty())
						{
							std::cerr<<"Dequeuing from empty ERROR--\n";
								break;
						}
						buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
					}
				}
				else
				{
					uint32_t goback_seq = seq / m_chunk*m_chunk;
					if (buffer_seq > goback_seq)
					{
						std::cout << "ERROR: Sendingbuffer miss!\n";
					}
					while (goback_seq > buffer_seq)
					{
						m_sendingBuffer[i]->Dequeue();
						if (m_sendingBuffer[i]->IsEmpty())
						{
							std::cerr<<"Dequeuing from empty ERROR--\n";
								break;
						}
						buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
					}

				}
			}

			m_queue->RecoverQueue(m_sendingBuffer[i], i);
			if (m_enable_window && i!=0 && !m_queue->m_queues[i]->IsEmpty())
			{
				curr_seq[i] = GetSeq(m_queue->m_queues[i]->Peek()->Copy());
			}


			PointToPointReceive(packet);
		}
		else if (ipv4h.GetProtocol() == 0xFC)//ACK on NIC
		{
			qbbHeader qbbh;
			p->Copy()->RemoveHeader(qbbh);

			int qIndex = qbbh.GetPG();
			int seq = qbbh.GetSeq();
			int port = qbbh.GetPort();
			int i;
			if (queue_index_map.find(port) == queue_index_map.end() || queue_index_map[port].find(qIndex) == queue_index_map[port].end())
			{
				std::cout << "ERROR: ACK NIC cannot find the flow\n";
				return ;
			}
			else{
				i = queue_index_map[port][qIndex];
			}
			
			m_acked[i] = seq;
			if (!m_sendingBuffer[i]->IsEmpty())
			{
				uint32_t buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());

				if (m_ack_interval == 0)
				{
					std::cout << "ERROR: shouldn't receive ack\n";
				}
				else
				{
					if (!m_backto0)
					{
						while (seq > buffer_seq)
						{
							m_sendingBuffer[i]->Dequeue();
							if (m_sendingBuffer[i]->IsEmpty())
							{
								break;
							}
							buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
						}
					}
					else
					{
						uint32_t goback_seq = seq / m_chunk*m_chunk;
						while (goback_seq > buffer_seq)
						{
							m_sendingBuffer[i]->Dequeue();
							if (m_sendingBuffer[i]->IsEmpty())
							{
								break;
							}
							buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
						}
					}
	                if(seq>=m_lastseq[i] && m_available_queue[i]) {
	                    std::cout<<"Flow finished at qbbid "<<qbbid<< " at "<<Simulator::Now()<<" at port"<<port<<" Num packets:"<<m_lastseq[i]<<", Time taken :"<<Simulator::Now()-starting_times[port]<<" qindex "<< qIndex<<" \n";
	                    queue_index_map[port].erase(qIndex);
	                    while(!m_sendingBuffer[i]->IsEmpty()) m_sendingBuffer[i]->Dequeue();
	                    available_sending_queues.push(i);
			    NS_ASSERT(egress_physicalQ_head_virtualQ_Index[i].size() == 1);
			    egress_physicalQ_head_virtualQ_Index[i].pop();
	                }

				}
			}


			PointToPointReceive(packet);
		}
	}

	void
		QbbNetDevice::PointToPointReceive(Ptr<Packet> packet)
	{
		NS_LOG_FUNCTION(this << packet);
		uint16_t protocol = 0;

		if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
		{
			// 
			// If we have an error model and it indicates that it is time to lose a
			// corrupted packet, don't forward this packet up, let it go.
			//
			m_phyRxDropTrace(packet);
		}
		else
		{
			// 
			// Hit the trace hooks.  All of these hooks are in the same place in this 
			// device becuase it is so simple, but this is not usually the case in 
			// more complicated devices.
			//
			// m_snifferTrace(packet);
			// m_promiscSnifferTrace(packet);
			// m_phyRxEndTrace(packet);
			//
			// Strip off the point-to-point protocol header and forward this packet
			// up the protocol stack.  Since this is a simple point-to-point link,
			// there is no difference in what the promisc callback sees and what the
			// normal receive callback sees.
			//
			ProcessHeader(packet, protocol);

			if (!m_promiscCallback.IsNull())
			{
				m_macPromiscRxTrace(packet);
				m_promiscCallback(this, packet, protocol, GetRemote(), GetAddress(), NetDevice::PACKET_HOST);
			}

			//m_macRxTrace(packet);
			m_rxCallback(this, packet, protocol, GetRemote());
		}
	}

	uint32_t
		QbbNetDevice::GetPriority(Ptr<Packet> p) //Pay attention this function modifies the packet!!! Copy the packet before passing in.
	{
		UdpHeader udph;
		p->RemoveHeader(udph);
		SeqTsHeader seqh;
		p->RemoveHeader(seqh);
		//NS_ASSERT(seqh.GetPG(), seqh.GethqCnt_qCnt)
		return seqh.GetPG();
	}

	uint32_t
		QbbNetDevice::GetSeq(Ptr<Packet> p) //Pay attention this function modifies the packet!!! Copy the packet before passing in.
	{
		uint16_t protocol;
		ProcessHeader(p, protocol);
		Ipv4Header ipv4h;
		p->RemoveHeader(ipv4h);
		UdpHeader udph;
		p->RemoveHeader(udph);
		SeqTsHeader seqh;
		p->RemoveHeader(seqh);
		return seqh.GetSeq();
	}
	
	SeqTsHeader
		QbbNetDevice::GetSth(Ptr<Packet> p) //Pay attention this function modifies the packet!!! Copy the packet before passing in.
	{
		uint16_t protocol;
		ProcessHeader(p, protocol);
		Ipv4Header ipv4h;
		p->RemoveHeader(ipv4h);
		UdpHeader udph;
		p->RemoveHeader(udph);
		SeqTsHeader seqh;
		p->RemoveHeader(seqh);
		return seqh;
	}

	Ipv4Address
		QbbNetDevice::GetSource(Ptr<Packet> p) //Pay attention this function modifies the packet!!! Copy the packet before passing in.
	{
		uint16_t protocol = 0;
		ProcessHeader(p, protocol);

		Ipv4Header ipv4h;
		p->RemoveHeader(ipv4h);
		return ipv4h.GetSource();
	}

	uint32_t
		QbbNetDevice::GetPort(Ptr<Packet> p) //Pay attention this function modifies the packet!!! Copy the packet before passing in.
	{
		uint16_t protocol = 0;
		ProcessHeader(p, protocol);

		Ipv4Header ipv4h;
		p->RemoveHeader(ipv4h);
		UdpHeader udph;
		p->RemoveHeader(udph);
		// SeqTsHeader sth;
		// p->PeekHeader(sth);

		// p->AddHeader(udph);
		return udph.GetSourcePort();
	}

	bool
		QbbNetDevice::Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
	{
		// NS_LOG_FUNCTION(this << packet << dest << protocolNumber);
		// NS_LOG_LOGIC("UID is " << packet->GetUid());
		if (IsLinkUp() == false) {
			m_macTxDropTrace(packet);
			return false;
		}

		Ipv4Header h;
		packet->PeekHeader(h);
		unsigned qIndex;
		if (h.GetProtocol() == 0xFF || h.GetProtocol() == 0xFE || h.GetProtocol() == 0xFD || h.GetProtocol() == 0xFC )  //QCN or PFC or NACK, go highest priority
		{
			qIndex = qCnt - 1;
		}
		else
		{
			Ptr<Packet> p = packet->Copy();
			p->RemoveHeader(h);

			if (h.GetProtocol() == 17)
				qIndex = GetPriority(p);
			else
				qIndex = 1; //dctcp
		}

		Ptr<Packet> p = packet->Copy();
		AddHeader(packet, protocolNumber);

		if (m_node->GetNodeType() == 0)
		{
			
			if (qIndex == qCnt - 1)
			{
				EnqueueQbb(packet, 0); //QCN uses 0 as the highest priority on NIC
			}
			else
			{
				Ipv4Header ipv4h;
				p->RemoveHeader(ipv4h);
				UdpHeader udph;
				p->RemoveHeader(udph);
				uint32_t port = udph.GetSourcePort();
				uint32_t i;
                if(queue_index_map.find(port) == queue_index_map.end() || queue_index_map[port].find(qIndex) == queue_index_map[port].end()) 
                {
                	if(!available_sending_queues.empty())
                	{
                		i = available_sending_queues.front();
				available_sending_queues.pop();
				if (queue_index_map.find(port) == queue_index_map.end() || queue_index_map[port].find(qIndex) == queue_index_map[port].end()) {
				} else {
					std::cout<<"Reusing existing stuff"<<"\n";
					NS_ASSERT(false);
				}
					    m_available_queue[i] = false;
			       		m_findex_qindex_map[i] = qIndex;
			        	queue_index_map[port][qIndex] = i;
			        	starting_times[port] = Simulator::Now();

			        	//m_rate[i] = m_bps;
			        	std::cout<<"Starting flow at "<<starting_times[port]<<" port "<<port<<" qbbid "<<qbbid<<" qindex "<<qIndex<<" IP "<<ipv4h.GetSource()<<" \n";

						curr_seq[i] = 0;
						m_acked[i] = 0;

						
				    	NS_ASSERT(i < fCnt);
					NS_ASSERT (i!=0);
					uint32_t p_hqCnt = GethqCnt(packet, true, 0);
					std::cout<<"Vq "<<p_hqCnt<<"\n";
					//NS_ASSERT(p_hqCnt != 0);
					NS_ASSERT(egress_physicalQ_head_virtualQ_Index[i].size() == 0);
					egress_physicalQ_head_virtualQ_Index[i].push(i);
					NS_ASSERT(egress_physicalQ_head_virtualQ_Index[i].size() == 1);
					}
					else{
						std::cerr<<"ERROR Out of sender queues\n";
						NS_ASSERT(false);
					}
                }
                else{
                	i = queue_index_map[port][qIndex];
                }
			
				if (m_sendingBuffer[i]->GetNPackets() == 8000)
				{
					m_sendingBuffer[i]->Dequeue();
				}
				EnqueueQbb(packet, i);// Here we are enqueuing at both places--CHECK??
				m_sendingBuffer[i]->Enqueue(packet->Copy());

				if(m_enable_window && m_queue->GetNBytes(i)>0)
				{
					curr_seq[i] = GetSeq(m_queue->m_queues[i]->Peek()->Copy());
				}
			}

			DequeueAndTransmit();
		}
		else //switch
		{
			if ((qIndex != (qCnt -1)) && (packet->GetSize() < 100)) {
				qIndex = qCnt - 1;
				std::cout<<"Unexpected packet size "<<packet->GetSize()<<" Protocol: "<<h.GetProtocol()<<"\n";
			}
			//AddHeader(packet, protocolNumber);
			if (qIndex != qCnt - 1)			//not pause frame
			{
				uint32_t prev_assigned_queue = GethqCntqCnt(packet);
				FlowIdTag t;
				packet->PeekPacketTag(t);
				uint32_t inDev = t.GetFlowId();
				uint32_t p_hqCnt = GethqCnt(packet, true, m_ifIndex);//inDev);//GethqCntqCnt(packet));
				SeqTsHeader sth = GetSth(packet->Copy());
				uint32_t seq = sth.GetSeq();
				uint32_t EndSeq = sth.GetEndSeq();
				//uint32_t seq = GetSeq(packet->Copy());
				GetCentralArrayIndex(p_hqCnt);
				qIndex = m_node->m_broadcom->central_entry_array[p_hqCnt].physicalQ;
				NS_ASSERT(qIndex != 2 && qIndex != qCnt - 2);
				if (m_node->m_broadcom->CheckIngressAdmission(inDev, qIndex, packet->GetSize()) && m_node->m_broadcom->CheckEgressAdmission(m_ifIndex, qIndex, packet->GetSize()))			// Admission control
				{
					m_node->m_broadcom->UpdateIngressAdmission(inDev, qIndex, packet->GetSize(), m_ifIndex);
					m_node->m_broadcom->UpdateEgressAdmission(m_ifIndex, qIndex, packet->GetSize());			
						
						NS_ASSERT(m_node->m_broadcom->central_entry_array[p_hqCnt].size < 64000);
						m_node->m_broadcom->central_entry_array[p_hqCnt].size += 1;
					//m_macTxTrace(packet);
					EnqueueQbb(packet, qIndex); // go into MMU and queues
					NS_ASSERT(qIndex<fCnt);
					bool pause_on_packet = false;
					if ( m_node->m_broadcom->central_entry_array[p_hqCnt].size>1) {
                				pause_on_packet = UpdatePauseState(inDev, qIndex, prev_assigned_queue);
						Ptr<Ipv4> m_ipv4 = m_node->GetObject<Ipv4>();
						Ptr<NetDevice> device = m_ipv4->GetNetDevice(inDev);
			                        if (pause_on_packet) device->UpdatePauseQueue(qIndex, prev_assigned_queue);
					}
					if (pause_on_packet) {
						egress_physicalQ_head_virtualQ_Index[qIndex].push(qIndex + 100000);
					} else {
						egress_physicalQ_head_virtualQ_Index[qIndex].push(qIndex);
					}
					Update_Paused_PhysicalQ(qIndex);
				}
				DequeueAndTransmit();
			}
			else			//pause or cnp, doesn't need admission control, just go
			{
				EnqueueQbb(packet, qIndex);
				DequeueAndTransmit();
			}

		}
		return true;
	}

    void
        QbbNetDevice::SendPauseFrame(std::vector<int> pClasses[])//uint64_t pause_map_a, uint64_t pause_map_b)
    {


		Ptr<Packet> p = Create<Packet>(0);
		uint16_t pause_size = (bloom_filter_length)/16;
		uint16_t pause_diff[100];
		for (uint16_t i = 0; i < pause_size; i++) {
			pause_diff [i] = bloom_filter[i%((bloom_filter_length)/16)];
		bool run_pfc = false;
		if (run_pfc) {
			if (m_node->m_broadcom->m_usedIngressPortBytes[m_ifIndex] > ((1/8.0)*(m_node->m_broadcom->m_maxBufferBytes - m_node->m_broadcom->m_usedTotalBytes)))
                               pause_diff[i] = 65535;
	               if (m_node->m_broadcom->m_usedIngressPortBytes[m_ifIndex] > ((1/8.0)*(m_node->m_broadcom->m_maxBufferBytes - m_node->m_broadcom->m_usedTotalBytes))) {
                     		std::cout<<"Stat id "<<qbbid<<" T "<<Simulator::Now()<<" P-";
		       }
               }

		}
		PauseHeader pauseh(m_pausetime, pause_size, pause_diff); //Change
		p->AddHeader(pauseh);
		Ipv4Header ipv4h;  // Prepare IPv4 header
		ipv4h.SetProtocol(0xFE);
		ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
		ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
		ipv4h.SetPayloadSize(p->GetSize());
	    	ipv4h.SetTtl(1);
	    	ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
		p->AddHeader(ipv4h);
		Send(p, Mac48Address("ff:ff:ff:ff:ff:ff"), 0x0800);
    }



	bool
		QbbNetDevice::IsLocal(const Ipv4Address& addr) const
	{
		Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>();
		for (unsigned i = 0; i < ipv4->GetNInterfaces(); i++) {
			for (unsigned j = 0; j < ipv4->GetNAddresses(i); j++) {
				if (ipv4->GetAddress(i, j).GetLocal() == addr) {
					return true;
				};
			};
		};
		return false;
	}

	void
		QbbNetDevice::ConnectWithoutContext(const CallbackBase& callback)
	{
		NS_LOG_FUNCTION(this);
		m_sendCb.ConnectWithoutContext(callback);
	}

	void
		QbbNetDevice::DisconnectWithoutContext(const CallbackBase& callback)
	{
		NS_LOG_FUNCTION(this);
		m_sendCb.DisconnectWithoutContext(callback);
	}

	int32_t
		QbbNetDevice::PrintStatus(std::ostream& os)
	{
		os << "Size:";
		uint32_t sum = 0;
		for (unsigned i = 0; i < qCnt; ++i) {
			os << " " << (m_paused[i] ? "q" : "Q") << "[" << i << "]=" << m_queue->GetNBytes(i);
			sum += m_queue->GetNBytes(i);
		};
		os << " sum=" << sum << std::endl;
		return sum;
};


	bool
		QbbNetDevice::Attach(Ptr<QbbChannel> ch)
	{
		NS_LOG_FUNCTION(this << &ch);
		m_channel = ch;
		m_channel->Attach(this);
		NotifyLinkUp();
		return true;
	}

	bool
		QbbNetDevice::TransmitStart(Ptr<Packet> p)
	{
		// NS_LOG_FUNCTION(this << p);
		// NS_LOG_LOGIC("UID is " << p->GetUid() << ")");
		//
		// This function is called to start the process of transmitting a packet.
		// We need to tell the channel that we've started wiggling the wire and
		// schedule an event that will be executed when the transmission is complete.
		//
		NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
		m_txMachineState = BUSY;
		m_currentPkt = p;
		//m_phyTxBeginTrace(m_currentPkt);
		Time txTime = Seconds(m_bps.CalculateTxTime(p->GetSize()));
		Time txCompleteTime = txTime + m_tInterframeGap;
		//NS_LOG_LOGIC("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds() << "sec");
		Simulator::Schedule(txCompleteTime, &QbbNetDevice::TransmitComplete, this);

		bool result = m_channel->TransmitStart(p, this, txTime);
		if (result == false)
		{
			m_phyTxDropTrace(p);
		}
		return result;
	}

	Address
		QbbNetDevice::GetRemote(void) const
	{
		NS_ASSERT(m_channel->GetNDevices() == 2);
		for (uint32_t i = 0; i < m_channel->GetNDevices(); ++i)
		{
			Ptr<NetDevice> tmp = m_channel->GetDevice(i);
			if (tmp != this)
			{
				return tmp->GetAddress();
			}
		}
		NS_ASSERT(false);
		return Address();  // quiet compiler.
	}

	void
		QbbNetDevice::SetBroadcomParams(
			uint32_t pausetime,
			double qcn_interval,
			double qcn_resume_interval,
			double g,
			DataRate minRate,
			DataRate rai,
			uint32_t fastrecover_times
		)
	{
		m_pausetime = pausetime;
	}

	Ptr<Channel>
		QbbNetDevice::GetChannel(void) const
	{
		return m_channel;
	}

	uint32_t
		QbbNetDevice::GetUsedBuffer(uint32_t port, uint32_t qIndex)
	{
		uint32_t i;
            if(queue_index_map.find(port)==queue_index_map.end() || queue_index_map[port].find(qIndex)==queue_index_map[port].end()) return 0;
            i = queue_index_map[port][qIndex];
			return m_queue->GetNBytes(i);
	}


	void
		QbbNetDevice::SetQueue(Ptr<BEgressQueue> q)
	{
		NS_LOG_FUNCTION(this << q);
		m_queue = q;
	}

	Ptr<BEgressQueue>
		QbbNetDevice::GetQueue()
	{
		return m_queue;
	}


	int
		QbbNetDevice::ReceiverCheckSeq(uint32_t seq, uint32_t key, uint32_t fseq, uint32_t port, Ipv4Address src, uint32_t qIndex, uint32_t size)
	{
		uint32_t expected = ReceiverNextExpectedSeq[key];

		if (seq == expected)
		{
			ReceiverNextExpectedSeq[key] = expected + 1;
            if (seq == fseq - 1){
                ReceiverNextExpectedSeq[key]=fseq+1;
                m_available_receiving[key] = true;
                receiver_index_map[src.Get()][port].erase(qIndex);
                available_receiving_queues.insert(key);
                std::cout<<"Received the last packet on port "<<port<<" at "<<Simulator::Now()<<" qbbid "<<qbbid<<" IP "<<src<<" size "<<size<<" \n";
                return 1;
            }
			if (ReceiverNextExpectedSeq[key] > m_milestone_rx[key])
			{
				m_milestone_rx[key] += m_ack_interval;
				return 1; //Generate ACK
			}
			else if (ReceiverNextExpectedSeq[key] % m_chunk == 0)
			{
				return 1;
			}
			else
			{
				return 5;
			}

		}
		else if (seq > expected)
		{
			std::cout<<"Don't know how to handle out of order packets\n";
			NS_ASSERT(false);
			std::cout<<"Generated NACK\n";
			// Generate NACK
			if (Simulator::Now() > m_nackTimer[key] || m_lastNACK[key] != expected)
			{
				m_nackTimer[key] = Simulator::Now() + MicroSeconds(m_nack_interval);
				m_lastNACK[key] = expected;
				if (m_backto0 && !m_testRead)
				{
					ReceiverNextExpectedSeq[key] = ReceiverNextExpectedSeq[key] / m_chunk*m_chunk;
				}
				return 2;
			}
			else
				return 4;
		}
		else
		{
			// Duplicate. 
			return 3;
		}
	}
	void
		QbbNetDevice::FlowFinished(uint32_t port, uint32_t pg, uint32_t end_seq) {
			//NS_ASSERT(false);
           uint32_t i=1;
           if(queue_index_map.find(port)==queue_index_map.end() || queue_index_map[port].find(pg)==queue_index_map[port].end())
           {
           		std::cerr<<"ERROR---Finishing a flow that does not exist\n";
           		return;
           }
           i = queue_index_map[port][pg];
	  NS_ASSERT(i!=0);

           m_available_queue[i] = true;
           m_lastseq[i]=end_seq;
        }

      bool
		QbbNetDevice::UpdatePauseState(uint32_t inDev, uint32_t qIndex, uint32_t p_hqCnt)
		{
			Ptr<Ipv4> m_ipv4 = m_node->GetObject<Ipv4>();
			Ptr<NetDevice> device = m_ipv4->GetNetDevice(inDev);
			double mark_up = 1.5;
		
			m_usedQueues = 0;
			for (int i = 0; i < ((qCnt - 1) / 64) + 1; i++) {
				m_usedQueues += __builtin_popcountll(~paused_map[i] & m_queue->active_queues_arr[i]);
			}
			if (~paused_map[(qCnt-1)/64] & power_of_two[(qCnt-1)%64] & m_queue->active_queues_arr[(qCnt-1)/64]) m_usedQueues -= 1;
			if (m_usedQueues == 0)
			{
				m_usedQueues = 1;
			}

				double threshold;
				threshold = mark_up * ((m_bps.GetBitRate() * 2 * 0.001 * 0.001) / (1.0*m_usedQueues));
		   if ((m_queue->GetNBytes(qIndex) * 8 > threshold)) 
	            {
			    return true;
	            } else {
			    return false;
                    }
		}

		void
		QbbNetDevice::UpdatePauseQueue(uint32_t qIndex, uint32_t p_hqCnt)
		{
			if(qIndex==0 || qIndex==qCnt-1)
			{
				NS_ASSERT(false);
				std::cerr<<"Update states at wrong places\n";
			}
			uint32_t index = p_hqCnt % bloom_filter_length;
			if (bloom_filter_counter[index] == 0)
				bloom_filter[index/16] |= uint16_t(power_of_two[index%16]);
			bloom_filter_counter[index] += 1;
			return;
		}

		void
		QbbNetDevice::UpdateResumeQueue(uint32_t qIndex, uint32_t p_hqCnt)
		{
			if(qIndex==0 || qIndex==qCnt-1)
			{
				NS_ASSERT(false);
				std::cerr<<"Update states at wrong places\n";
			}
			bool send_resume = true;
			uint32_t index = p_hqCnt % bloom_filter_length;
			if (bloom_filter_counter[index] == 1) {
				bloom_filter[index/16] &= ~uint16_t(power_of_two[index%16]);
			} else {
				send_resume = false;
			}
			NS_ASSERT(bloom_filter_counter[index] > 0);
			bloom_filter_counter[index] -= 1;
			if (send_resume) {
				std::vector<int> empty[1];
				SendPauseFrame(empty);
			}
			return;
		}

		bool
		QbbNetDevice::is_queue_paused(uint32_t qIndex)
		{
			return ((paused_map[qIndex/64] & (power_of_two[qIndex%64]))!=0);
		}

		bool QbbNetDevice::EnqueueQbb(Ptr<Packet> p,uint32_t qIndex) {
			if (qIndex == 0 || m_node->GetNodeType() == 1) //Careful
				return m_queue->Enqueue(p, qIndex);
			PppHeader ppp;
  			p->RemoveHeader (ppp);
			Ipv4Header ipv4h;
			p->RemoveHeader(ipv4h);
			UdpHeader udph;
                	p->RemoveHeader(udph);
                	SeqTsHeader sth;
                	p->RemoveHeader(sth);
                	sth.SethqCnt_qCnt(qIndex);
                	p->AddHeader(sth);
                	p->AddHeader(udph);
			p->AddHeader(ipv4h);
			p->AddHeader(ppp);
			return m_queue->Enqueue(p, qIndex);
		}
		uint32_t QbbNetDevice::GethqCnt(Ptr<Packet> p, bool eq, uint32_t outDev) {
			Ptr<Packet> pCopy = p->Copy();;
			PppHeader ppp;
			Ipv4Header ipv4h;
			if (eq)
			{
  				pCopy->RemoveHeader (ppp);
				pCopy->RemoveHeader(ipv4h);
			}

			uint32_t prio = GetPriority(pCopy->Copy());
			UdpHeader udph;
                	pCopy->RemoveHeader(udph);
                	SeqTsHeader sth;
                	pCopy->RemoveHeader(sth);
			uint32_t source_int = (ipv4h.GetSource()).Get();
			uint32_t ret = ((source_int*1235 + udph.GetSourcePort() + 13*sth.GetEndSeq())%(hqCnt/pCnt));
			ret = ret*pCnt + outDev;
			return ret;
		}
		uint32_t QbbNetDevice::Assign_hqCnt_qCnt(uint32_t p_hqCnt) {
			uint32_t qIndex;
			bool found = false;
			uint32_t seed = rand();//p_hqCnt;
			for (int i = 0; i < qCnt; i++) {
				uint32_t qid = (seed + i) % qCnt;
				if (qid < 3 || qid > qCnt-3) {
					continue;
				}
				if (m_queue->GetNBytes(qid) == 0 && !is_queue_paused(qid)) {
					found = true;
					qIndex = qid;
					break;
				}
			}
			if(!found) {
				srand(p_hqCnt);
				qIndex = 3 + (rand()%(qCnt-5));
			}
			NS_ASSERT(qIndex != 2 && qIndex != qCnt - 2);
			//NS_ASSERT(qIndex < 255);
			return qIndex;
		}
		uint32_t QbbNetDevice::GethqCntqCnt(Ptr<Packet> p) {
			//return 0;
			Ptr<Packet> pCopy = p->Copy();;
			PppHeader ppp;
  			pCopy->RemoveHeader (ppp);
			//uint16_t protocol;
			//ProcessHeader(pCopy, protocol);
			Ipv4Header ipv4h;
			pCopy->RemoveHeader(ipv4h);
			UdpHeader udph;
                	pCopy->RemoveHeader(udph);
                	SeqTsHeader sth;
                	pCopy->RemoveHeader(sth);
			return sth.GethqCnt_qCnt();
		}
		void QbbNetDevice::Update_Paused_PhysicalQ(uint32_t physicalQ) {
			bool paused = false;
			if (m_node->GetNodeType() == 0 && egress_physicalQ_head_virtualQ_Index[physicalQ].size() == 0) {
				paused = false;
			}
			else {
				uint32_t index = physicalQ % bloom_filter_length;
				if (received_bloom_filter[index/16] & uint16_t(power_of_two[index%16])) {
					paused = true;
				}
				
			}
			if (paused) {
				paused_map[physicalQ/64] |= power_of_two[physicalQ%64];
			} else {
				paused_map[physicalQ/64] &= ~power_of_two[physicalQ%64];
			}
		}
		void QbbNetDevice::GetCentralArrayIndex(uint32_t p_hqCnt) {
			NS_ASSERT(hqCnt == m_node->m_broadcom->hqCnt);
			NS_ASSERT(p_hqCnt < hqCnt);
			if (m_node->m_broadcom->central_entry_array[p_hqCnt].size == 0 && !m_node->m_broadcom->central_entry_array[p_hqCnt].paused) {
				if ((Simulator::Now().GetSeconds() - m_node->m_broadcom->central_entry_array[p_hqCnt].last_time) > 0.000006) {
					m_node->m_broadcom->central_entry_array[p_hqCnt].physicalQ = Assign_hqCnt_qCnt(p_hqCnt);
				}
			}
			m_node->m_broadcom->central_entry_array[p_hqCnt].last_time = Simulator::Now().GetSeconds();
		}

} // namespace ns3

