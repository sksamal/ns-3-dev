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

//#define NEED_ANIMATION
#define EXPORT_STATS
#define NEED_TRACE

/*
	- This work is based on help taken from "Towards Reproducible Performance Studies of Datacenter Network Architectures Using An Open-Source Simulation Approach" and HyScale Paper

	- The code is constructed in the following order:
		1. Creation of Node Containers 
		2. Initialize settings for On/Off Application
		3. Initialize hosts and allocate their addresses
		4. Connect the switches according to the description of topology
		5. Start Simulation

	- Addressing scheme:
		1. Address of host: 10.level.switch.0 /24 (level=0)
		2. Address of BCube switch: 10.level.switch.0 /16

	- On/Off Traffic of the simulation: addresses of client and server are randomly selected everytime	

	- Simulation Settings:
                - Number of HyScale levels: k (input parameter)
                - Number of hosts under each switch (num_hosts): 2
		- Simulation running time: 100 seconds
		- Packet size: 1024 bytes
		- Data rate for packet sending: 1 Mbps
		- Data rate for device channel: 1000 Mbps
		- Delay time for device: 0.001 ms
		- Communication pairs selection: Random Selection with uniform probability
		- Traffic flow pattern: Exponential random traffic
		- Routing protocol: Nix-Vector
        
        - Statistics Output:
                - Flowmonitor XML output file: HyScale.xml is located in the /statistics folder
		- Ascii Tracefile : HyScale.tr is located in the /statistics folder 
		- NetAnim Tracefile : hyScale-netanim.xml is located in the /statistics folder 
            
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

// Generate Address in reverse order - used for efficiency in Hyscale
// Decimal to X base
 int genHyscaleAddressReverse(int number, int X,int *lsb) {
   int result=0;
   *lsb = number%X;
   while(number>0)  //number in base 10
   {
     result = (number%X)+ result*10;  // reverse order
     number = number/X;
   }
   return result;
}

// Print HyScale Address 
   string FormattedHyScaleAdd(int number,int level) {
     char address[10]; // 10 levels
     std::sprintf(address,"<%d",number%10); 
     number=number/10;
     while(number>0)
     {    
     std::sprintf(address,"%s,%d",address,number%10);
     number=number/10;
     level--;
     }
     while(level>0)
     	{ 
	  std::sprintf(address,"%s,0",address);
	  level--;
        }
     std::sprintf(address,"%s>",address);
     return string(address);
}

// Main function
//
int	main(int argc, char *argv[])
{

  	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

	//Open the stats file if needed
#ifdef EXPORT_STATS
        ofstream sfile;
 	sfile.open ("statistics/hyscale-stats.csv", ios::out | ios::app);
#endif
//=========== Define parameters based on value of k ===========//
//
	int k;
	CommandLine cmd;
    	cmd.AddValue("k", "Number of levels in HyScale", k);  // Take from input
	cmd.Parse (argc, argv);

//	int k = 1;			// number of levels (recursive depth)
	int num_hosts = 2;		// number of hosts per rack
//	int num_bridge = 1;		// number of bridges per rack (one each per node)
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
        strcat(traceFile,".tr");// filename for Ascii trace output file

// Initialize other variables
//
	int i = 0;		
	int j = 0;				
	int temp = 0;		

// Initialize the addresses and lsbs based on 8-bit base
// Based on Hyscale address scheme
	for(i=0;i<total_hosts;i++)
	  addresses[i]=	genHyscaleAddressReverse(i,num_agg,&lsb[i]);

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
	host.Create (total_hosts);				
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
	ApplicationContainer app[total_hosts];
	for (i=0;i<total_hosts;i++){

		// Randomly select a server
		levelRand = rand() % total_agg ;
		swRand = rand() % num_agg + 0;
		hostRand = rand() % num_hosts + 0;
		hostRand = hostRand+2;
		char *add;
		add = toString(10, levelRand, swRand, hostRand);

		cout<<"Address="<<add<<endl;

	// Initialize On/Off Application with addresss of server
		OnOffHelper oo = OnOffHelper("ns3::UdpSocketFactory",Address(InetSocketAddress(Ipv4Address(add), port))); // ip address of server
	        oo.SetAttribute("OnTime",StringValue ("ns3::ExponentialRandomVariable[Mean=1.0|Bound=0.0]"));  
	        oo.SetAttribute("OffTime",StringValue ("ns3::ExponentialRandomVariable[Mean=1.0|Bound=0.0]"));  
 	        oo.SetAttribute("PacketSize",UintegerValue (packetSize));
 	       	oo.SetAttribute("DataRate",StringValue (dataRate_OnOff));      
	        oo.SetAttribute("MaxBytes",StringValue (maxBytes));

		// Randomly select a client
		// to make sure that client and server are different
		randHost = rand() % num_hosts + 0;		
		int temp = num_hosts*swRand + (hostRand-2);
		while (temp== randHost){
			randHost = rand() % num_hosts + 0;
		} 

		cout<<"RandHost="<<randHost<<endl;
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
	NetDeviceContainer hostSwDevices[total_bridges];		
	NetDeviceContainer bridgeDevices[total_bridges];		
	Ipv4InterfaceContainer ipContainer[total_bridges];

	temp = 0;
        int l=0;
	for (i=0;i<total_agg;i++){
	   for (j=0;j<num_agg;j++){
		int k = i*num_agg+j;
		NetDeviceContainer link1 = csma.Install(NodeContainer (switches.Get(k), bridges.Get(k)));
		hostSwDevices[k].Add(link1.Get(0));			
		bridgeDevices[k].Add(link1.Get(1));			
		temp = l;
		for (l=temp;l<temp+num_hosts; l++){
			NetDeviceContainer link2 = csma.Install(NodeContainer (host.Get(l), bridges.Get(k)));
			hostSwDevices[k].Add(link2.Get(0));		
			bridgeDevices[k].Add(link2.Get(1));			 
		}	
		BridgeHelper bHelper;
		bHelper.Install (bridges.Get(k), bridgeDevices[k]);	
		//Assign address
		char *subnet;
		subnet = toString(10, i, j, 0);
//		std::cout <<"Subnet="<<subnet<<endl;
		address.SetBase (subnet, "255.255.255.0");
		ipContainer[k] = address.Assign(hostSwDevices[k]);	
	}
	}
	std::cout <<"Fininshed connecting switches to hosts"<<"\n";

//===============Interconnect the switches in Hyscale=============//
//
//	Not the efficient way though - Check each pair of switches to 
//	see if they need to be connected based on the formula
//
	NetDeviceContainer ae[total_bridges][total_bridges]; 	
	for (i=0;i<total_bridges;i++){
		int m=0;
		for(j=0;j<total_bridges;j++) {

			if(i==j) continue;

//			cout<<"Evaluating i & j"<<i<<" "<<j<<endl;
			int addI=addresses[i];
			int lsbI=lsb[i];
			int addJ=addresses[j];
			int lsbJ=lsb[j];
			//cout<<"Add(i)=<"<<FormattedHyScaleAdd(addI,k)<<">,Add(j)=<"<<FormattedHyScaleAdd(addJ,k)<<">"<<endl;
			//Recursive  - High Complexity - Unfortunate
			while(addI>0 && addJ>0) {
			  int msbI=addI%10;
			  int msbJ=addJ%10;
			//	As per definition
			  for(int z=0;z<4;z++) {
				
			   int f1i=(2*z+1+msbI)%8;
			   int f2i=(z+(i%8)/2)%8;
			   int f3i=(f2i+8/2)%8;

  			   // Connect if either of this is true 
  			   if((f1i==msbJ && f2i==lsbI) || (f3i==lsbI && lsbI==lsbJ)) {
				ae[i][m++] = p2p.Install(switches.Get(i), switches.Get(j));
			cout<<"Add(i)="<<FormattedHyScaleAdd(addI,k)<<",Add(j)="<<FormattedHyScaleAdd(addJ,k)<<endl;
				cout<<i<<"-"<<j<<"connected"<<endl;
			    }		   
			}			
			addI= (addI==0) ? -1 : addI/10;
			addJ= (addJ==0) ? -1 : addJ/10;
		}		
	}
}

//=========== Start the simulation ===========//
//

	std::cout << "Start Simulation.. "<<"\n";
	for (i=0;i<total_hosts;i++){
		app[i].Start (Seconds (0.0));
  		app[i].Stop (Seconds (100.0));
	}
  	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

// Calculate Throughput using Flowmonitor
//
  	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();
// Run simulation.
//

#ifdef NEED_TRACE
  	AsciiTraceHelper ascii;
  	csma.EnableAsciiAll (ascii.CreateFileStream (traceFile));
#endif

#ifdef NEED_ANIMATION
	// TO BE IMPLEMENTED
        std::string animFile = "statistics/hyscale-netanim.xml";
        AnimationInterface anim(animFile);
#endif

  	NS_LOG_INFO ("Run Simulation.");
  	Simulator::Stop (Seconds(100.0));
  	Simulator::Run ();

	// Export statistics
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

	int txPackets=0,rxPackets=0,lostPackets=0;
        ns3::Time delaySum = NanoSeconds(0.0);
        ns3::Time jitterSum = NanoSeconds(0.0);
        ns3::Time lastDelay = NanoSeconds(0.0);
        int timesForwarded=0;
        double averageDelay;
        double throughput; 
	int nFlows=0;
	cout<<"Before calculating stats"<<endl;
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
  	{
	  nFlows++;
	  txPackets+=iter->second.txPackets;
	  rxPackets+=iter->second.rxPackets;
	  lostPackets+=iter->second.lostPackets;
	  delaySum+=iter->second.delaySum;
	  jitterSum+=iter->second.jitterSum;
	  lastDelay+=iter->second.lastDelay;
	  timesForwarded+=iter->second.timesForwarded;
	  averageDelay+=iter->second.delaySum.GetNanoSeconds()/iter->second.rxPackets;
	  throughput+=iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024;
/*	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

 	  NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
    	  NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
    	  NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
    	  NS_LOG_UNCOND("DelaySum = " << iter->second.delaySum);
    	  NS_LOG_UNCOND("JitterSum = " << iter->second.jitterSum);
    	  NS_LOG_UNCOND("LastDelay = " << iter->second.lastDelay);
    	  NS_LOG_UNCOND("Lost Packets = " << iter->second.lostPackets);
    	  NS_LOG_UNCOND("timesForwarded = " << iter->second.timesForwarded);
    	  NS_LOG_UNCOND("Average Delay = " << iter->second.delaySum/iter->second.rxPackets);
    	  NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps"); */
  }

	cout<<"After calculating stats"<<endl;

#ifdef EXPORT_STATS
	sfile<<"HyScale"<<","<<k<<","<<nFlows<<","<<txPackets<<","<<rxPackets<<","<<delaySum<<","<<jitterSum<<","<<lastDelay;
 	sfile<<","<<lostPackets<<","<<timesForwarded<<","<<averageDelay/nFlows<<","<<throughput/nFlows<<endl;
	//sfile<<"HyScale"<<","<<"k"<<","<<"txPackets"<<","<<"rxPackets"<<","<<"delaySum"<<","<<"jitterSum"<<","<<"lastDelay";
// 	sfile<<","<<"lostPackets"<<","<<"timesForwarded"<<","<<"averageDelay"<<","<<"throughput"<<endl;
	sfile.close();
#endif
	std::cout<<"HyScale"<<","<<"k"<<","<<"txPackets"<<","<<"rxPackets"<<","<<"delaySum"<<","<<"jitterSum"<<","<<"lastDelay";
 	std::cout<<","<<"lostPackets"<<","<<"timesForwarded"<<","<<"averageDelay"<<","<<"throughput"<<endl;
	std::cout<<"HyScale"<<","<<k<<","<<txPackets<<","<<rxPackets<<","<<delaySum<<","<<jitterSum<<","<<lastDelay;
 	std::cout<<","<<lostPackets<<","<<timesForwarded<<","<<averageDelay/nFlows<<","<<throughput/nFlows<<endl;
  	monitor->SerializeToXmlFile(filename, true, true);
	std::cout << "Simulation finished "<<"\n";
  	Simulator::Destroy ();
  	monitor->CheckForLostPackets ();
  	monitor->SerializeToXmlFile(filename, true, false);

  	NS_LOG_INFO ("Done.");

	return 0;
}
