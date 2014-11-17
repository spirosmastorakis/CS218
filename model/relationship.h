#ifndef RELATIONSHIP_H
#define RELATIONSHIP_H

#include "ns3/nstime.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"


#define ARRAY_SIZE 1000
#define K_CLUSTER 8 // TODO Increase the number of clusters
#define COMM_NUM 5 // TODO Increasae the number of communities

namespace ns3
{

struct Peer
{
    Ipv4Address ID;
    uint32_t commID;
};

struct BorderNode
{
    Ipv4Address ID;
    uint32_t count;
};

struct SocialTableEntry
{
    SocialTableEntry ():
        timestamp (Seconds(0)), socialTieValue (0) {}
    Peer peer1;
    Peer peer2;
    Time timestamp;
    double socialTieValue;
};

struct CentralityTableEntry
{
    CentralityTableEntry():
        centrality (0), SocialLevel (0){}
    Peer node;
    double centrality;
    int SocialLevel;
};

struct Community
{
    Community():nodeNum (0) {}
    uint32_t commID;
    int nodeNum;
};

class Relationship
{
public:
    Relationship ();
    Relationship (Peer peer);
    void UpdateAndMergeSocialTable (Peer peer, Time time,
            SocialTableEntry *socialTableAddress, uint32_t size);
    double GetSocialTie (Ipv4Address peer1, Ipv4Address peer2) const;
    uint32_t GetSocialTableSize () const;
    uint32_t GetCentralityTableSize () const;
    Ipv4Address *GetHigherCentralityNodes (uint32_t &size);
    //CentralityTableEntry *GetHigherCentralityNodes (uint32_t &size);
    CentralityTableEntry *GetCentralityTableAddress (uint32_t &size); // Use this for the entire table
    SocialTableEntry *GetSocialTableAddress ();
    int *GetCentralityArrayAddress (int &size); // Use this one for Array of integers
    Ipv4Address GetHigherSocialTie (Ipv4Address peer1, Ipv4Address peer2, Ipv4Address destination); // Return the peer with higher social tie to the destination
    Ipv4Address GetHigherSocialLevel (Ipv4Address peer1, Ipv4Address peer2); // Return peer with the higher social level
    Ipv4Address GetBestBorderNode (uint32_t commID);
    Ipv4Address GetHigherCentralityNode (Ipv4Address peer1, Ipv4Address peer2);
	bool HasHighestSocialLevel (Ipv4Address targetNodeID, uint32_t commID);

    
    // Test functions
    void PrintCentralityTable ();
    void PrintSocialTable ();
    void PrintSocialLevelTable ();
    void PrintCommunityList ();

    Peer me;
   
private:
    void UpdateSocialTable (Peer peer, Time time);
    void MergeSocialTables(SocialTableEntry* peerTable, uint32_t size);
    int SearchTableEntry (Ipv4Address peer1, Ipv4Address peer2);
    double ComputeSocialTie (int index, Time time);
    void ComputeCentrality ();
    uint32_t UniqueNodeCount (uint32_t commID);
    bool PeerCounted (BorderNode bn [], Ipv4Address node, uint32_t size);
    bool PeerCompared (Ipv4Address pc [], Ipv4Address node, uint32_t size);
    void DetermineCommunities ();
    void BuildCommunityTable (Community comm);
    

private:
    SocialTableEntry socialTable [ARRAY_SIZE];
    CentralityTableEntry centralityTable [ARRAY_SIZE];
    Community communityList [COMM_NUM];
    //CentralityTableEntry higherCentralityTable [ARRAY_SIZE/2];
    Ipv4Address higherCentralityTable [ARRAY_SIZE/2];
    CentralityTableEntry * communityCentralityTable[ARRAY_SIZE/2];
    double centrality;
    uint32_t socialTableSize;
    uint32_t centralityTableSize;
    uint32_t communityListSize;
    int centralityArray [ARRAY_SIZE];
    double centers [K_CLUSTER];

//    Peer me;
};

} // namespace ns3

#endif // RELATIONSHIP_H
