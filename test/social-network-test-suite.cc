/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// Include a header file from your module to test.
#include "ns3/qhs.h"

// An essential include is test.h
#include "ns3/test.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

// This is an example TestCase.
class SocialNetworkTestCase1 : public TestCase
{
public:
  SocialNetworkTestCase1 ();
  virtual ~SocialNetworkTestCase1 ();

private:
  virtual void DoRun (void);
};

// Add some help text to this case to describe what it is intended to test
SocialNetworkTestCase1::SocialNetworkTestCase1 ()
  : TestCase ("SocialNetwork test case (does nothing)")
{
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
SocialNetworkTestCase1::~SocialNetworkTestCase1 ()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
SocialNetworkTestCase1::DoRun (void)
{
  // A wide variety of test macros are available in src/core/test.h
  NS_TEST_ASSERT_MSG_EQ (true, true, "true doesn't equal true for some reason");
  // Use this one for floating point comparisons
  //NS_TEST_ASSERT_MSG_EQ_TOL (0.01, 0.01, 0.001, "Numbers are not equal within tolerance");
  /*
  Relationship relationship;
  std::vector<Ipv4Address> fringeNodes;
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
  socialTable[1].peer1.commID = uint32_t (1);
  socialTable[1].peer2.ID = Ipv4Address ("1.1.1.1");
  socialTable[1].peer2.commID = uint32_t (0);
  socialTable[1].socialTieValue = double (0.4);

  //Entry3 = (SocialTableEntry*) malloc (sizeof(SocialTableEntry));
  socialTable[2].peer1.ID = Ipv4Address ("2.1.1.2");
  socialTable[2].peer1.commID = uint32_t (1);
  socialTable[2].peer2.ID = Ipv4Address ("1.1.1.1");
  socialTable[2].peer2.commID = uint32_t (0);
  socialTable[2].socialTieValue = double (0.6);

  uint32_t socialTieTableSize = uint32_t (3);
  // SocialTableEntry *socialTableEntry = (SocialTableEntry *) (socialTable); 

  QHS qhs (0);
  fringeNodes = qhs.UpdateFringeNodeSet (&socialTable[0], socialTieTableSize);

  NS_TEST_ASSERT_MSG_EQ(fringeNodes.size(), 2,  "the packets do not match");
*/
}

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class SocialNetworkTestSuite : public TestSuite
{
public:
  SocialNetworkTestSuite ();
};

SocialNetworkTestSuite::SocialNetworkTestSuite ()
  : TestSuite ("social-network", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new SocialNetworkTestCase1, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static SocialNetworkTestSuite socialNetworkTestSuite;

