/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"

 
 using namespace ns3;
 
 NS_LOG_COMPONENT_DEFINE("Task4");
 
 int
 main(int argc, char* argv[])
 {
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");

    //создание enb и ue
    NodeContainer enbNodes;
    enbNodes.Create(1);
    NodeContainer ueNodes;
    ueNodes.Create(2);

    //расположение
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> UEposition = CreateObject<ListPositionAllocator> ();
    UEposition->Add(Vector (1.0,1.0, 0.0));
    UEposition->Add(Vector (1.0,-1.0, 0.0));
    mobility.SetPositionAllocator(UEposition);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ueNodes);

    //установка стека лте
    NetDeviceContainer enbDevs;
    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueDevs;
    ueDevs = lteHelper->InstallUeDevice(ueNodes);
    lteHelper->Attach(ueDevs, enbDevs.Get(0));


    InternetStackHelper internet;
    internet.Install(ueNodes);

    //установка ip стека
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));

   //настройка маршрута
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    for (uint32_t u = 0; u < ueNodes.GetN(); u++)
    {
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(
            ueNodes.Get(u)->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    uint16_t port0 = 9000; 
    uint16_t port1 = 9001; 
    
    //UE0 слушает порт0 (UE1)
    PacketSinkHelper sinkHelper0("ns3::UdpSocketFactory",
                            InetSocketAddress(Ipv4Address::GetAny(), port0));
    ApplicationContainer sinkApp0 = sinkHelper0.Install(ueNodes.Get(0));
    sinkApp0.Start(Seconds(0.1));
    sinkApp0.Stop(Seconds(1.0)); 

    //UE1 слушает порт1 (UE0)
    PacketSinkHelper sinkHelper1("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port1));
    ApplicationContainer sinkApp1 = sinkHelper1.Install(ueNodes.Get(1));
    sinkApp1.Start(Seconds(0.1));
    sinkApp1.Stop(Seconds(1.0));    
    
    //UE1 отправляет пакеты на порт0
    UdpClientHelper client0(ueIpIface.GetAddress(0),port0);
    client0.SetAttribute("MaxPackets", UintegerValue(1000000)); 
    client0.SetAttribute("Interval", TimeValue(Time("0.0001s"))); 
    client0.SetAttribute("PacketSize", UintegerValue(1000)); 
    ApplicationContainer clientApp0 = client0.Install(ueNodes.Get(1));
    clientApp0.Start(Seconds(0.1));
    clientApp0.Stop(Seconds(1.0));   
    
    //UE0 отправляет пакеты на порт1
    UdpClientHelper client1(ueIpIface.GetAddress(1), port1);
    client1.SetAttribute("MaxPackets", UintegerValue(1000000)); // почти бесконечно
    client1.SetAttribute("Interval", TimeValue(Time("0.0001s"))); // минимальный интервал
    client1.SetAttribute("PacketSize", UintegerValue(1000)); // размер пакета в байтах
    ApplicationContainer clientApp1 = client1.Install(ueNodes.Get(0));
    clientApp1.Start(Seconds(0.1));
    clientApp1.Stop(Seconds(1.0));   

    //сьем статистики
    lteHelper->EnableRlcTraces();
    lteHelper->EnableMacTraces();


    Simulator::Stop(Seconds(1.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
 }
 
