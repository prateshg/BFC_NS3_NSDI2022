/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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

#include <iostream>

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/qbb-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-remote-channel.h"
#include "ns3/qbb-channel.h"
#include "ns3/qbb-remote-channel.h"
#include "ns3/queue.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/mpi-interface.h"
#include "ns3/mpi-receiver.h"

#include "ns3/trace-helper.h"
#include "point-to-point-helper.h"
#include "qbb-helper.h"
#include "ns3/custom-header.h"
#include "trace-format.h"
#include <vector>

NS_LOG_COMPONENT_DEFINE ("QbbHelper");

namespace ns3 {

QbbHelper::QbbHelper ()
{
  m_queueFactory.SetTypeId ("ns3::DropTailQueue");
  m_deviceFactory.SetTypeId ("ns3::QbbNetDevice");
  m_channelFactory.SetTypeId ("ns3::QbbChannel");
  m_remoteChannelFactory.SetTypeId ("ns3::QbbRemoteChannel");
}

void 
QbbHelper::SetQueue (std::string type,
                              std::string n1, const AttributeValue &v1,
                              std::string n2, const AttributeValue &v2,
                              std::string n3, const AttributeValue &v3,
                              std::string n4, const AttributeValue &v4)
{
  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
}

void 
QbbHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void 
QbbHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  m_channelFactory.Set (n1, v1);
  m_remoteChannelFactory.Set (n1, v1);
}

void 
QbbHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type QbbNetDevice.
  //
  Ptr<QbbNetDevice> device = nd->GetObject<QbbNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("QbbHelper::EnablePcapInternal(): Device " << device << " not of type ns3::QbbNetDevice");
      return;
    }

  PcapHelper pcapHelper;

  std::string filename;
  if (explicitFilename)
    {
      filename = prefix;
    }
  else
    {
      filename = pcapHelper.GetFilenameFromDevice (prefix, device);
    }

  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out, 
                                                     PcapHelper::DLT_PPP);
  pcapHelper.HookDefaultSink<QbbNetDevice> (device, "PromiscSniffer", file);
}

void 
QbbHelper::EnableAsciiInternal (
  Ptr<OutputStreamWrapper> stream, 
  std::string prefix, 
  Ptr<NetDevice> nd,
  bool explicitFilename)
{
  //
  // All of the ascii enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type QbbNetDevice.
  //
  Ptr<QbbNetDevice> device = nd->GetObject<QbbNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("QbbHelper::EnableAsciiInternal(): Device " << device << 
                   " not of type ns3::QbbNetDevice");
      return;
    }

  //
  // Our default trace sinks are going to use packet printing, so we have to 
  // make sure that is turned on.
  //
  Packet::EnablePrinting ();

  //
  // If we are not provided an OutputStreamWrapper, we are expected to create 
  // one using the usual trace filename conventions and do a Hook*WithoutContext
  // since there will be one file per context and therefore the context would
  // be redundant.
  //
  if (stream == 0)
    {
      //
      // Set up an output stream object to deal with private ofstream copy 
      // constructor and lifetime issues.  Let the helper decide the actual
      // name of the file given the prefix.
      //
      AsciiTraceHelper asciiTraceHelper;

      std::string filename;
      if (explicitFilename)
        {
          filename = prefix;
        }
      else
        {
          filename = asciiTraceHelper.GetFilenameFromDevice (prefix, device);
        }

      Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream (filename);

      //
      // The MacRx trace source provides our "r" event.
      //
      asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<QbbNetDevice> (device, "MacRx", theStream);

      //
      // The "+", '-', and 'd' events are driven by trace sources actually in the
      // transmit queue.
      //
	  
	  //std::cout<<"Hook Callback\n";

      Ptr<BEgressQueue> queue = device->GetQueue ();
      asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<BEgressQueue> (queue, "Enqueue", theStream);
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<BEgressQueue> (queue, "Drop", theStream);
      asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<BEgressQueue> (queue, "Dequeue", theStream);

      // PhyRxDrop trace source for "d" event
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<QbbNetDevice> (device, "PhyRxDrop", theStream);

      return;
    }

  //
  // If we are provided an OutputStreamWrapper, we are expected to use it, and
  // to providd a context.  We are free to come up with our own context if we
  // want, and use the AsciiTraceHelper Hook*WithContext functions, but for 
  // compatibility and simplicity, we just use Config::Connect and let it deal
  // with the context.
  //
  // Note that we are going to use the default trace sinks provided by the 
  // ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
  // but the default trace sinks are actually publicly available static 
  // functions that are always there waiting for just such a case.
  //
  uint32_t nodeid = nd->GetNode ()->GetId ();
  uint32_t deviceid = nd->GetIfIndex ();
  std::ostringstream oss;

  oss << "/NodeList/" << nd->GetNode ()->GetId () << "/DeviceList/" << deviceid << "/$ns3::QbbNetDevice/MacRx";
  //oss << "/N/" << nd->GetNode ()->GetId () << "/D/" << deviceid << "/$ns3::QbbNetDevice/MacRx";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::QbbNetDevice/TxBeQueue/Enqueue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::QbbNetDevice/TxBeQueue/Dequeue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::QbbNetDevice/TxBeQueue/Drop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::QbbNetDevice/PhyRxDrop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

NetDeviceContainer 
QbbHelper::Install (NodeContainer c)
{
  NS_ASSERT (c.GetN () == 2);
  return Install (c.Get (0), c.Get (1));
}

NetDeviceContainer 
QbbHelper::Install (Ptr<Node> a, Ptr<Node> b)
{
  NetDeviceContainer container;

  Ptr<QbbNetDevice> devA = m_deviceFactory.Create<QbbNetDevice> ();
  devA->SetAddress (Mac48Address::Allocate ());
  a->AddDevice (devA);
  Ptr<QbbNetDevice> devB = m_deviceFactory.Create<QbbNetDevice> ();
  devB->SetAddress (Mac48Address::Allocate ());
  b->AddDevice (devB);
  
  Ptr<BEgressQueue> queueA = CreateObject<BEgressQueue> ();
  devA->SetQueue (queueA);
  Ptr<BEgressQueue> queueB = CreateObject<BEgressQueue> ();
  devB->SetQueue (queueB);


  // If MPI is enabled, we need to see if both nodes have the same system id 
  // (rank), and the rank is the same as this instance.  If both are true, 
  //use a normal p2p channel, otherwise use a remote channel
  bool useNormalChannel = true;
  Ptr<QbbChannel> channel = 0;
  if (MpiInterface::IsEnabled ())
    {
      uint32_t n1SystemId = a->GetSystemId ();
      uint32_t n2SystemId = b->GetSystemId ();
      uint32_t currSystemId = MpiInterface::GetSystemId ();
      if (n1SystemId != currSystemId || n2SystemId != currSystemId) 
        {
          useNormalChannel = false;
        }
    }
  if (useNormalChannel)
    {
      channel = m_channelFactory.Create<QbbChannel> ();
    }
  else
    {
      channel = m_remoteChannelFactory.Create<QbbRemoteChannel> ();
      Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver> ();
      Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver> ();
      mpiRecA->SetReceiveCallback (MakeCallback (&QbbNetDevice::Receive, devA));
      mpiRecB->SetReceiveCallback (MakeCallback (&QbbNetDevice::Receive, devB));
      devA->AggregateObject (mpiRecA);
      devB->AggregateObject (mpiRecB);
    }

  devA->Attach (channel);
  devB->Attach (channel);
  container.Add (devA);
  container.Add (devB);

  return container;
}

NetDeviceContainer 
QbbHelper::Install (Ptr<Node> a, std::string bName)
{
  Ptr<Node> b = Names::Find<Node> (bName);
  return Install (a, b);
}

NetDeviceContainer 
QbbHelper::Install (std::string aName, Ptr<Node> b)
{
  Ptr<Node> a = Names::Find<Node> (aName);
  return Install (a, b);
}

NetDeviceContainer 
QbbHelper::Install (std::string aName, std::string bName)
{
  Ptr<Node> a = Names::Find<Node> (aName);
  Ptr<Node> b = Names::Find<Node> (bName);
  return Install (a, b);
}

void QbbHelper::PacketEventCallback(FILE *file, Ptr<NetDevice> dev, Ptr<const Packet> p, uint32_t qidx, Event event, bool hasL2){
	TraceFormat tr;

	CustomHeader hdr((hasL2?CustomHeader::L2_Header:0) | CustomHeader::L3_Header | CustomHeader::L4_Header);
	p->PeekHeader(hdr);

	tr.event = event;
	tr.node = dev->GetNode()->GetId();
	tr.intf = dev->GetIfIndex();
	tr.qidx = qidx;
	tr.time = Simulator::Now().GetTimeStep();
	tr.sip = hdr.sip;
	tr.dip = hdr.dip;
	tr.l3Prot = hdr.l3Prot;
	tr.ecn = hdr.m_tos & 0x3;
	switch (hdr.l3Prot){
		case 0x6:
			tr.data.sport = hdr.tcp.sport;
			tr.data.dport = hdr.tcp.dport;
			break;
		case 0x11:
			tr.data.sport = hdr.udp.sport;
			tr.data.dport = hdr.udp.dport;
			// SeqTsHeader
			tr.data.seq = hdr.udp.seq;
			tr.data.ts = hdr.udp.ts;
			tr.data.pg = hdr.udp.pg;
			tr.data.endseq = hdr.udp.endseq;
			tr.data.hqCnt_qCnt = hdr.udp.hqCnt_qCnt;
			break;
		case 0xFC:
		case 0xFD:
			tr.ack.pg = hdr.ack.pg;
			tr.ack.seq = hdr.ack.seq;
			tr.ack.port = hdr.ack.port;
			break;
		case 0xFE:
			tr.pfc.time = hdr.pfc.time;
			tr.pfc.num = hdr.pfc.num;
			//tr.pfc.pause_map.clear();
			NS_ASSERT(tr.pfc.num < 100);
			for (uint16_t i = 0; i < tr.pfc.num; i++) { 
				tr.pfc.pause_map[i] = hdr.pfc.pause_map[i];
			}
			/*tr.pfc.pause_map_a = hdr.pfc.pause_map_a;
			tr.pfc.pause_map_b = hdr.pfc.pause_map_b;
			tr.pfc.pause_map_c = hdr.pfc.pause_map_c;
			tr.pfc.pause_map_d = hdr.pfc.pause_map_d;
			tr.pfc.pause_map_e = hdr.pfc.pause_map_e;
			tr.pfc.pause_map_f = hdr.pfc.pause_map_f;
			tr.pfc.pause_map_g = hdr.pfc.pause_map_g;
			tr.pfc.pause_map_h = hdr.pfc.pause_map_h;
			tr.pfc.pause_map_i = hdr.pfc.pause_map_i;
			tr.pfc.pause_map_j = hdr.pfc.pause_map_j;
			tr.pfc.pause_map_k = hdr.pfc.pause_map_k;
			tr.pfc.pause_map_l = hdr.pfc.pause_map_l;
			tr.pfc.pause_map_m = hdr.pfc.pause_map_m;
			tr.pfc.pause_map_n = hdr.pfc.pause_map_n;
			tr.pfc.pause_map_o = hdr.pfc.pause_map_o;
			tr.pfc.pause_map_p = hdr.pfc.pause_map_p;*/
			break;
		case 0xFF:
			tr.cnp.fid = hdr.cnp.fid;
			tr.cnp.qIndex = hdr.cnp.qIndex;
			tr.cnp.qfb = hdr.cnp.qfb;
			tr.cnp.ecnBits = hdr.cnp.ecnBits;
			tr.cnp.total = hdr.cnp.total;
			break;
		default:
			break;
	}
	tr.size = p->GetSize();//hdr.m_payloadSize;
	tr.qlen = DynamicCast<QbbNetDevice>(dev)->GetQueue()->GetNBytes(qidx);
	fwrite(&tr, sizeof(tr), 1, file);
}

void QbbHelper::MacRxDetailCallback (FILE* file, Ptr<NetDevice> dev, Ptr<const Packet> p){
	PacketEventCallback(file, dev, p, 0, Recv, false);
}

void QbbHelper::EnqueueDetailCallback(FILE* file, Ptr<NetDevice> dev, Ptr<const Packet> p, uint32_t qidx){
	PacketEventCallback(file, dev, p, qidx, Enqu, true);
}

void QbbHelper::DequeueDetailCallback(FILE* file, Ptr<NetDevice> dev, Ptr<const Packet> p, uint32_t qidx){
	PacketEventCallback(file, dev, p, qidx, Dequ, true);
}

void QbbHelper::EnableTracingDevice(FILE *file, Ptr<NetDevice> nd){
	uint32_t nodeid = nd->GetNode ()->GetId ();
	uint32_t deviceid = nd->GetIfIndex ();
	std::ostringstream oss;

	oss << "/NodeList/" << nd->GetNode ()->GetId () << "/DeviceList/" << deviceid << "/$ns3::QbbNetDevice/MacRx";
	Config::ConnectWithoutContext (oss.str (), MakeBoundCallback (&QbbHelper::MacRxDetailCallback, file, nd));

	oss.str ("");
	oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::QbbNetDevice/TxBeQueue/BeqEnqueue";
	Config::ConnectWithoutContext (oss.str (), MakeBoundCallback (&QbbHelper::EnqueueDetailCallback, file, nd));

	oss.str ("");
	oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::QbbNetDevice/TxBeQueue/BeqDequeue";
	Config::ConnectWithoutContext (oss.str (), MakeBoundCallback (&QbbHelper::DequeueDetailCallback, file, nd));

#if 0
	oss.str ("");
	oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::QbbNetDevice/TxBeQueue/Drop";
	Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

	oss.str ("");
	oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::QbbNetDevice/PhyRxDrop";
	Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
#endif
}

void QbbHelper::EnableTracing(FILE *file, NodeContainer node_container){
  NetDeviceContainer devs;
  for (NodeContainer::Iterator i = node_container.Begin (); i != node_container.End (); ++i)
    {
      Ptr<Node> node = *i;
      for (uint32_t j = 0; j < node->GetNDevices (); ++j)
        {
		  EnableTracingDevice(file, node->GetDevice(j));
        }
    }
}

} // namespace ns3
