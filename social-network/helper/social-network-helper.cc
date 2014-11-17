/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "social-network-helper.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/string.h"

using namespace std;

namespace ns3 {

SocialNetworkHelper::SocialNetworkHelper (uint16_t port)
{
    m_factory.SetTypeId (SocialNetwork::GetTypeId ());
    SetAttribute ("RemotePort", UintegerValue (port));
}

SocialNetworkHelper::SocialNetworkHelper (uint16_t port, Ipv4Address requestedContent)
{
    m_factory.SetTypeId (SocialNetwork::GetTypeId ());
    SetAttribute ("RemotePort", UintegerValue (port));
    SetAttribute ("RequestedContent",  Ipv4AddressValue (requestedContent));
}

void 
SocialNetworkHelper::SetAttribute (
    std::string name, 
    const AttributeValue &value)
{
    m_factory.Set (name, value);
}

void
SocialNetworkHelper::SetFill (Ptr<Application> app, std::string fill)
{
    app->GetObject<SocialNetwork>()->SetFill (fill);
}

void
SocialNetworkHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
    app->GetObject<SocialNetwork>()->SetFill (fill, dataLength);
}

void
SocialNetworkHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
    app->GetObject<SocialNetwork>()->SetFill (fill, fillLength, dataLength);
}

ApplicationContainer
SocialNetworkHelper::Install (Ptr<Node> node) const
{
    return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SocialNetworkHelper::Install (std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node> (nodeName);
    return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SocialNetworkHelper::Install (NodeContainer c) const
{
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
        apps.Add (InstallPriv (*i));
    }

    return apps;
}

Ptr<Application>
SocialNetworkHelper::InstallPriv (Ptr<Node> node) const
{
    Ptr<Application> app = m_factory.Create<SocialNetwork> ();
    node->AddApplication (app);

    return app;
}

}

