/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

// Network topology
//
//       n0    n1   n2   n3   n4   n5   n6   n7
//       |     |    |    |    |    |    |    |
//       =====================================
//              LAN
//
// - UDP flows from n0 to n1 and back
// - DropTail queues 
// - Tracing of queues and packet receptions to file "udp-echo.tr"

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/rng-seed-manager.h"
#include <time.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("UdpEchoExample");

int networks(int argc, char *argv[],int runno)
{
//
// Users may find it convenient to turn on explicit debugging
// for selected modules; the below lines suggest how to do this
//
#if 0
  LogComponentEnable ("UdpEchoExample", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);
#endif
//
// Allow the user to override any of the defaults and the above Bind() at
// run-time, via command-line arguments
//
//

  RngSeedManager::SetSeed((unsigned int)time(NULL));
  CommandLine cmd;
  bool useV6 = false;
  Address serverAddress_6,serverAddress_3,serverAddress_4,serverAddress_5;
  uint32_t packetSize = atoi(argv[3]);
  uint32_t maxPacketCount=atoi(argv[1]);
  Time interPacketInterval = Seconds(atof(argv[2]));

  cmd.AddValue("maxPacketCount", "maxPackets",maxPacketCount);
  cmd.AddValue("interPacketInterval", "Interval",interPacketInterval);
  cmd.AddValue ("packetSize", "packetSize", packetSize);
  cmd.AddValue ("useIpv6", "Use Ipv6", useV6);

  cmd.Parse (argc, argv);

  
//
// Explicitly create the nodes required by the topology (shown above).
//
  NS_LOG_INFO ("Create nodes.");
  NodeContainer n;
  n.Create (8);

  InternetStackHelper internet;
  internet.Install (n);

  NS_LOG_INFO ("Create channels.");
//
// Explicitly create the channels required by the topology (shown above).
//
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (1048576)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
  NetDeviceContainer d = csma.Install (n);

//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  NS_LOG_INFO ("Assign IP Addresses.");
  if (useV6 == false)
    {
      Ipv4AddressHelper ipv4;
      ipv4.SetBase ("10.1.1.0", "255.255.255.0");
      Ipv4InterfaceContainer i = ipv4.Assign (d);
      serverAddress_5 = Address(i.GetAddress (4));
      serverAddress_6 = Address(i.GetAddress (5));
      serverAddress_3 = Address(i.GetAddress (2));
      serverAddress_4 = Address(i.GetAddress (3));
    }
  else
    {
      /*Ipv6AddressHelper ipv6;
      ipv6.SetBase ("2001:0000:f00d:cafe::", Ipv6Prefix (64));
      Ipv6InterfaceContainer i6 = ipv6.Assign (d);
      serverAddress = Address(i6.GetAddress (1,1));*/
    }

  NS_LOG_INFO ("Create Applications.");
//
// Create a UdpEchoServer application on node one.
//
  uint16_t port = 9;  // well-known echo port number
  UdpEchoServerHelper server_5 (port),server_6 (port),server_3 (port),server_4 (port);
  ApplicationContainer apps_5 = server_5.Install (n.Get (4)),apps_6 = server_6.Install (n.Get (5)),
                       apps_3 = server_3.Install (n.Get (2)),apps_4 = server_4.Install (n.Get (3));
  apps_5.Start (Seconds (1.0));
  apps_6.Start (Seconds (1.0));
  apps_3.Start (Seconds (1.0));
  apps_4.Start (Seconds (1.0));

  apps_5.Stop (Seconds (100.0));
  apps_6.Stop (Seconds (100.0));
  apps_3.Stop (Seconds (100.0));
  apps_4.Stop (Seconds (100.0));

//
// Create a UdpEchoClient application to send UDP datagrams from node zero to
// node one.
//
  //uint32_t packetSize = 1024;
  //uint32_t maxPacketCount = 1;
  //Time interPacketInterval = Seconds (1.);
  UdpEchoClientHelper client_1 (serverAddress_5, port),client_2 (serverAddress_6, port),client_7 (serverAddress_3, port),client_8 (serverAddress_4, port);
  client_1.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client_2.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client_7.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client_8.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client_1.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client_2.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client_7.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client_8.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client_1.SetAttribute ("PacketSize", UintegerValue (packetSize));
  client_2.SetAttribute ("PacketSize", UintegerValue (packetSize));
  client_7.SetAttribute ("PacketSize", UintegerValue (packetSize));
  client_8.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps_5 = client_1.Install (n.Get (0));
  apps_6 = client_2.Install (n.Get (1));
  apps_3 = client_7.Install (n.Get (6));
  apps_4 = client_8.Install (n.Get (7));
  apps_5.Start (Seconds (2.0));
  apps_6.Start (Seconds (2.0));
  apps_3.Start (Seconds (2.0));
  apps_4.Start (Seconds (2.0));
  apps_5.Stop (Seconds (100.0));
  apps_6.Stop (Seconds (100.0));
  apps_3.Stop (Seconds (100.0));
  apps_4.Stop (Seconds (100.0));

#if 0
//
// Users may find it convenient to initialize echo packets with actual data;
// the below lines suggest how to do this
//
  client.SetFill (apps.Get (0), "Hello World");

  client.SetFill (apps.Get (0), 0xa5, 1024);

  uint8_t fill[] = { 0, 1, 2, 3, 4, 5, 6};
  client.SetFill (apps.Get (0), fill, sizeof(fill), 1024);
#endif
  std::string s1(argv[2]),s2(argv[3]);
  std::stringstream ss;
  ss<<runno;
  std::string s3;
  ss>>s3;
  std::string s="udp-echo-"+s1+"-"+s2+"-"+"run-"+s3;
  std::string t=s+".tr";
  std::cout<<s<<"\n";
  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream (t));
  //csma.EnablePcapAll (s, false);

//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
  return 0;
}

int main(int argc, char *argv[])
{
  for (int i = 0; i < 7; ++i)
  {
   if(i==0){
    strcpy(argv[1],"1000");
    strcpy(argv[2],"0.5");
    strcpy(argv[3],"8192");
   }
   if(i==1){
    strcpy(argv[1],"1000");
    strcpy(argv[2],"0.5");
    strcpy(argv[3],"16384");
   } 
   if(i==2){
    strcpy(argv[1],"1000");
    strcpy(argv[2],"0.5");
    strcpy(argv[3],"32768");
   } 
   if(i==3){
    strcpy(argv[1],"1000");
    strcpy(argv[2],"0.25");
    strcpy(argv[3],"32768");
   } 
   if(i==4){
    strcpy(argv[1],"1000");
    strcpy(argv[2],"0.125");
    strcpy(argv[3],"32768");
   } 
   if(i==5){
    strcpy(argv[1],"1000");
    strcpy(argv[2],"0.0625");
    strcpy(argv[3],"32768");
   } 
   if(i==6){
    strcpy(argv[1],"1000");
    strcpy(argv[2],"0.03125");
    strcpy(argv[3],"32768");
   }
   for (int j = 0; j < 10; ++j)
    {
      networks(argc,argv,j);
    }
}
  return 0;
}
