/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/qhs.h"
#include "ns3/relationship.h"
#include "ns3/ipv4-address.h" 

//#include "test-qhs.h"
//#include "ns3/test.h"
#include "ns3/test.h"
#include "ns3/ptr.h"

// Modify DoRun method to call your test script

// Run sudo NS_LOG=qhs ./waf --run "test-runner --suite=QHS-test"

using namespace ns3;

class MyTestCase : public TestCase
{
public:
  MyTestCase ();
  void FringeNodes (std::map<Ipv4Address, uint32_t> &fringeNodes);
  virtual void DoRun (void); 
};

MyTestCase::MyTestCase ()
  : TestCase ("fringe")
{
}

void MyTestCase::FringeNodes (std::map<Ipv4Address, uint32_t> &fringeNodes)
{
  Relationship relationship;
  fringeNodes.clear ();
  SocialTableEntry socialTable [2];

  uint32_t localCommunity = uint32_t (0);

  //Entry1 = (SocialTableEntry*) malloc (sizeof(SocialTableEntry));
  socialTable[0].peer1.ID = Ipv4Address ("1.1.1.1");
  socialTable[0].peer1.commID = uint32_t (0);
  socialTable[0].peer2.ID = Ipv4Address ("1.1.1.2");
  socialTable[0].peer2.commID = uint32_t (0);
  socialTable[0].socialTieValue = double (0.8);

  //Entry2 = (SocialTableEntry*) malloc (sizeof(SocialTableEntry));
  socialTable[1].peer1.ID = Ipv4Address ("2.1.1.1");
  socialTable[1].peer1.commID = uint32_t (0);
  socialTable[1].peer2.ID = Ipv4Address ("1.1.1.1");
  socialTable[1].peer2.commID = uint32_t (1);
  socialTable[1].socialTieValue = double (0.4);

  //Entry3 = (SocialTableEntry*) malloc (sizeof(SocialTableEntry));
  socialTable[2].peer1.ID = Ipv4Address ("1.1.1.3");
  socialTable[2].peer1.commID = uint32_t (0);
  socialTable[2].peer2.ID = Ipv4Address ("2.1.1.2");
  socialTable[2].peer2.commID = uint32_t (1);
  socialTable[2].socialTieValue = double (0.6);

  uint32_t socialTieTableSize = uint32_t (3);
  // SocialTableEntry *socialTableEntry = (SocialTableEntry *) (socialTable); 

  QHS qhs (uint32_t(0));
  qhs.UpdateFringeNodeSet (&socialTable[0], socialTieTableSize, fringeNodes);

  NS_TEST_ASSERT_MSG_EQ(fringeNodes.size (), 1,  "the packets do not match");
}

void MyTestCase::DoRun ()
{ 
  std::map<Ipv4Address, uint32_t> fringeNodes;
  FringeNodes (fringeNodes);
}

class QHSTest : public TestSuite
{
public:
        QHSTest ();
        //QHSTest() : TestCase("QHS test")
        //{
        //}
};
        QHSTest::QHSTest ()
          : TestSuite ("QHS-test", UNIT)
{
  AddTestCase (new MyTestCase);
}

static QHSTest qhsTest;
