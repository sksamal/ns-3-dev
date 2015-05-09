// Construction of HyScale Architecture
// Authors: Suraj Samal, Upasana Nayak

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 
 *
 */
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/flow-monitor-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/bridge-net-device.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"

/*
	- This work is based on help taken from "Towards Reproducible Performance Studies of Datacenter Network Architectures Using An Open-Source Simulation Approach" and HyScale Paper

	- The code is constructed in the following order:
		1. Creation of Node Containers 
		2. Initialize settings for On/Off Application
		3. Initialize hosts and their addresses
		4. Connect hosts level 1 switches
		5. Connect hosts level 2 switches
		6. Start Simulation

	- Addressing scheme:
		1. Address of host: 10.level.switch.0 /24
		2. Address of BCube switch: 10.level.switch.0 /16

	- On/Off Traffic of the simulation: addresses of client and server are randomly selected everytime	

	- Simulation Settings:
                - Number of nodes: 64-3375 (run the simulation with different values of n)
                - Number of HyScale levels: 3 (ie k=2 is fixed)
                - Number of nodes in BCube0 (n): 4-15
		- Simulation running time: 100 seconds
		- Packet size: 1024 bytes
		- Data rate for packet sending: 1 Mbps
		- Data rate for device channel: 1000 Mbps
		- Delay time for device: 0.001 ms
		- Communication pairs selection: Random Selection with uniform probability
		- Traffic flow pattern: Exponential random traffic
		- Routing protocol: Nix-Vector
        
        - Statistics Output:
                - Flowmonitor XML output file: BCube.xml is located in the /statistics folder
            
*/


using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("HyScale-Architecture");

// Function to create address string from numbers
//
char * toString(int a,int b, int c, int d){

	int first = a;
	int second = b;
	int third = c;
	int fourth = d;

	char *address =  new char[30];
	char firstOctet[30], secondOctet[30], thirdOctet[30], fourthOctet[30];	

	bzero(address,30);

	snprintf(firstOctet,10,"%d",first);
	strcat(firstOctet,".");
	snprintf(secondOctet,10,"%d",second);
	strcat(secondOctet,".");
	snprintf(thirdOctet,10,"%d",third);
	strcat(thirdOctet,".");
	snprintf(fourthOctet,10,"%d",fourth);

	strcat(thirdOctet,fourthOctet);
	strcat(secondOctet,thirdOctet);
	strcat(firstOctet,secondOctet);
	strcat(address,firstOctet);

	return address;
}

// Generate Address in reverse order - used for efficiency
// Decimal to X base
   int genAddressReverse(int number, int X,int *lsb) {
   int result;
   *lsb = number%X;
   while(number>0)  //number in base 10
   {
     result = (number%X)+ result*10;  // reverse order
     number = number/X;
   }
   return address;
}

// Main function
//
int	main(int argc, char *argv[])
{

  	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

//=========== Define parameters based on value of k ===========//
//
	int k;
	CommandLine cmd;
    	cmd.AddValue("k", "Number of levels in HyScale", k);
	cmd.Parse (argc, argv);

//	int k = 1;			// number of levels (recursive depth)
	int num_host = 2;		// number of hosts per rack
	int num_bridge = 1;		// number of bridges per rack (one each per node)
	int num_agg = 8;		// number of switches per basic topology
	int total_bridges = pow ( num_agg,k+1 );// number of switches per basic topology
	int total_hosts = num_hosts*total_bridges; // Total number of hosts
	int total_agg = total_bridges/num_agg; // Number of basic topologies
	char filename [] = "statistics/HyScale";
	char traceFile [] = "statistics/HyScale";
	int addresses[total_hosts];  // A 8 base address for Hyscale
	int lsb[total_hosts];  // To store lsb of base address for Hyscale

	// String operations for files 
	char buf[4];
	std::sprintf(buf,"-%d",k);
        strcat(filename,buf);
        strcat(filename,".xml");// filename for Flow Monitor xml output file
        strcat(traceFile,buf);
        strcat(traceFile,".tr");// filename for Flow Monitor xml output file

// Initialize other variables
//
	int i = 0;		
	int j = 0;				
	int temp = 0;		

// Initialize the addresses and lsbs based on 8-bit base
//
	for(i=0;i<num_hosts;i++)
 	{
	  addresses[i]=	genAddress(i,num_agg,&lsb[i]);
	  count<<i,<<address[i]<<endl;
 	}

// Define variables for On/Off Application
// These values will be used to serve the purpose that addresses of server and client are selected randomly
// Note: the format of host's address is 10.0.switch.host
//
	int levelRand = 0;	//	Always same for hyscale
	int swRand = 0;		// Random values for servers' address
	int hostRand = 0;	//
	int randHost =0;	// Random values for clients' address

// Initialize parameters for On/Off application
//
	int port = 9;
	int packetSize = 1024;		// 1024 bytes
	char dataRate_OnOff [] = "1Mbps";
	char maxBytes [] = "0";		// unlimited

// Initialize parameters for Csma protocol
//
	char dataRate [] = "1000Mbps";	// 1Gbps
	int delay = 0.001;		// 0.001 ms

// Output some useful information
//	
	std::cout << "Number of HyScale level =  "<< k+1<<"\n";
	std::cout << "Number of switch in basic HyScale level =  "<< num_agg<<"\n";
	std::cout << "Number of host under each switch =  "<< num_hosts <<"\n";
	std::cout << "Total number of host =  "<< total_hosts<<"\n";

// Initialize Internet Stack and Routing Protocols
//	
	InternetStackHelper internet;
	Ipv4NixVectorHelper nixRouting; 
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4ListRoutingHelper list;
	list.Add (staticRouting, 0);	
	list.Add (nixRouting, 10);	
	internet.SetRoutingHelper(list);	

//=========== Creation of Node Containers ===========//
//
	NodeContainer host;				// NodeContainer for hosts;  				
	host.Create (num_host);				
	internet.Install (host);			

	NodeContainer switches;				// NodeContainer for switches 
	switches.Create (total_bridges);				
	internet.Install (switches);
				
	NodeContainer bridges;				// NodeContainer for bridges
  	bridges.Create (total_bridges);
	internet.Install (bridges);


//=========== Initialize settings for On/Off Application ===========//
//

// Generate traffics for the simulation
//
	ApplicationContainer app[num_host];
	for (i=0;i<num_host;i++){

		// Randomly select a server
		levelRand = 0;
		swRand = rand() % total_agg + 0;
		hostRand = rand() % num_agg + 0;
		hostRand = hostRand+2;
		char *add;
		add = toString(10, levelRand, swRand, hostRand);

	// Initialize On/Off Application with addresss of server
		OnOffHelper oo = OnOffHelper("ns3::UdpSocketFactory",Address(InetSocketAddress(Ipv4Address(add), port))); // ip address of server
	        oo.SetAttribute("OnTime",StringValue ("ns3::ExponentialRandomVariable[Mean=1.0|Bound=0.0]"));  
	        oo.SetAttribute("OffTime",StringValue ("ns3::ExponentialRandomVariable[Mean=1.0|Bound=0.0]"));  
 	        oo.SetAttribute("PacketSize",UintegerValue (packetSize));
 	       	oo.SetAttribute("DataRate",StringValue (dataRate_OnOff));      
	        oo.SetAttribute("MaxBytes",StringValue (maxBytes));

		// Randomly select a client
		// to make sure that client and server are different
		randHost = rand() % num_host + 0;		
		int temp = n*swRand + (hostRand-2);
		while (temp== randHost){
			randHost = rand() % num_host + 0;
		} 

		// Install On/Off Application to the client
		NodeContainer onoff;
		onoff.Add(host.Get(randHost));
	     	app[i] = oo.Install (onoff);
	}

	std::cout << "Finished creating On/Off traffic"<<"\n";	

	// Inintialize Address Helper
	//	
  	Ipv4AddressHelper address;

	// Initialize PointtoPoint helper
	//	
	PointToPointHelper p2p;
  	p2p.SetDeviceAttribute ("DataRate", StringValue (dataRate));
  	p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (delay)));

	// Initialize Csma helper
	//
  	CsmaHelper csma;
  	csma.SetChannelAttribute ("DataRate", StringValue (dataRate));
  	csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (delay)));

	//=========== Connect switches to hosts ===========//
	//	
	NetDeviceContainer hostSwDevices[num_bridges];		
	NetDeviceContainer bridgeDevices[num_bridges];		
	Ipv4InterfaceContainer ipContainer[num_bridges];

	temp = 0;
	for (i=0;i<total_agg;i++){
	   for (j=0;j<num_agg;j++){
		int k = i*num_agg+j;
		NetDeviceContainer link1 = csma.Install(NodeContainer (switches.Get(k), bridges.Get(k)));
		hostSwDevices[k].Add(link1.Get(0));			
		bridgeDevices[k].Add(link1.Get(1));			
		temp = l;
		for (l=temp;l<temp+n; l++){
			NetDeviceContainer link2 = csma.Install(NodeContainer (host.Get(l), bridges.Get(k)));
			hostSwDevices[k].Add(link2.Get(0));		
			bridgeDevices[k].Add(link2.Get(1));			 
		}	
		BridgeHelper bHelper;
		bHelper.Install (bridges.Get(k), bridgeDevices[k]);	
		//Assign address
		char *subnet;
		subnet = toString(10, i, j, 0);
		address.SetBase (subnet, "255.255.255.0");
		ipContainer[k] = address.Assign(hostSwDevices[k]);	
	}
	std::cout <<"Fininshed connecting switches to hosts"<<"\n";

//===============Interconnect the switches in Hyscale=============//
//
//	Not the efficient way though
//
	NetDeviceContainer ae[num_bridges][num_bridges]; 	
//	Ipv4InterfaceContainer ipAeContainer[num_bridges];
	for (i=0;i<num_host;i++){
		int k=0;
		//Assign subnet
		char *subnet;
		subnet = toString(10, second_octet, third_octet, 0);
		//Assign base
		char *base;
		base = toString(0, 0, 0, fourth_octet);
		address.SetBase (subnet, "255.255.255.0",base);
		ipAeContainer[i][j][h] = address.Assign(ae[i][j][h]);
		for(j=0;j<num_host;j++) {

			if(i==j) continue;

			int addI=addresses[i];
			int lsbI=lsb[i];
			int addJ=addresses[j];
			int lsbJ=lsb[j];

			//Recursive  - High Complexity - Unfortunate
			while(addI>0 && addJ>0) {
			  int msbI=addI%10;
			//	As per definition
			  for(int z=0;z<4;z++) {
				
			   int f1i=(2*z+1+msbI)%8;
			   int f2i=(z+(i%8)/2)%8;
			   int f3i=(f2i+8/2)%8;

  			   // Connect if either of this is true 
  			   if(f1i==msbJ && f2i==lsbI) 
				ae[i][k++] = p2p.Install(switches.Get(i), switches.Get(j));
			   if(f3i==lsbI && lsbI==lsbJ)
				ae[i][k++] = p2p.Install(switches.Get(i), switches.Get(j));
			   
			}			
			addI=addI/10;
			addJ=addJ/10;
		}		
	}


//=========== Start the simulation ===========//
//

	std::cout << "Start Simulation.. "<<"\n";
	for (i=0;i<num_host;i++){
		app[i].Start (Seconds (0.0));
  		app[i].Stop (Seconds (10.0));
	}
  	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

// Calculate Throughput using Flowmonitor
//
  	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();
// Run simulation.
//
  	AsciiTraceHelper ascii;
  	csma.EnableAsciiAll (ascii.CreateFileStream (traceFile));

  	NS_LOG_INFO ("Run Simulation.");
  	Simulator::Stop (Seconds(11.0));
  	Simulator::Run ();

  	monitor->CheckForLostPackets ();
  	monitor->SerializeToXmlFile(filename, true, false);

	std::cout << "Simulation finished "<<"\n";

  	Simulator::Destroy ();
  	NS_LOG_INFO ("Done.");

	return 0;
}
