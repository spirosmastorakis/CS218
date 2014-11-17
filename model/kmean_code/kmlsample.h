

#ifndef KMS_H
#define KMS_H
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/relationship.h"

namespace ns3 
{

/*
typedef struct CentralityTableEntry
{
	Ipv4Address nodeId;
	double centrality;
	int SocialLevel;
}  CentralityTableEntry;
*/

void GenerateCenters(int, int * , int, int * );
int ListNodeWithHigherSocialLevel(int NodeSocialLevel, Ipv4Address *NodeList, CentralityTableEntry * myList, int ListLength);
int AssignGroup(int num1, double * centers, int NumCenters);
void GenerateCenters(int k, CentralityTableEntry * myList, int ValueLength, double * centers, int commId);


}
#endif
