#ifndef NDNSIM_TEST_NDN_NS3_H
#define NDNSIM_TEST_NDN_NS3_H

#include "ns3/test.h"
#include "ns3/ptr.h"



namespace ns3 {

class QHSTest : public TestCase
{
public:
	QHSTest() : TestCase("QHS test")
	{
	}

private:
	virtual void DoRun();

	void CheckFringeNodes();

};

}

#endif
