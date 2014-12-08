
#include "relationship.h"
#include "ns3/kmlsample.h"
#include <math.h>

namespace ns3
{

Relationship::Relationship (Peer peer):
    socialTableSize (0)
{
    me = peer;

}

Relationship::Relationship ():
   socialTableSize (0) 
{
}

uint32_t Relationship::GetSocialTableSize () const
{
    return socialTableSize;
}

int Relationship::SearchTableEntry (Ipv4Address peer1, Ipv4Address peer2)
{
    for (uint16_t i=0; i < socialTableSize; i++)
    {
        if ((peer1 == socialTable[i].peer1.ID) && (peer2 == socialTable[i].peer2.ID))
            return i;
    }
    return -1;
}

double Relationship::ComputeSocialTie (int index, Time baseTime)
{
    double F = 0;
    double lambda = 0.0001;
    double diffTime;
    
    // Compute time difference from encounter n and n-1
    diffTime = baseTime.GetSeconds () - socialTable[index].timestamp.GetSeconds (); 
    // Compute current weighing Function value
    F = pow(0.5, (diffTime * lambda));
    // Add weighing function to previous function
    return (F + socialTable[index].socialTieValue);
}

void Relationship::UpdateSocialTable (Peer peer2, Time time)
{
    SocialTableEntry s;
    s.peer1 = me;
    s.peer2 = peer2;
    s.timestamp = time;
    int index;

    // Determine if node was met in the past if less than 0 never met
    if ((index = SearchTableEntry (me.ID, peer2.ID)) < 0)
    {
        s.socialTieValue = 1;
        // Insert the encounter in the vector
        socialTable[socialTableSize] = s;
        socialTableSize = (socialTableSize + 1) % ARRAY_SIZE;
    }
    // node is in the vector
    else
    {
        // Determine socail tie value and place in vector
        s.socialTieValue = ComputeSocialTie (index, time);
        socialTable[index] = s;
    }
    
    // Swap the nodes
    s.peer1 = peer2;
    s.peer2 = me;

    // Determine if node was met in the past if less than 0 never met
    if ((index = SearchTableEntry (peer2.ID, me.ID)) < 0)
    {
        s.socialTieValue = 1;
        // Insert the encounter in the vector
        socialTable[socialTableSize] = s;
        socialTableSize = (socialTableSize + 1) % ARRAY_SIZE;
    }
    // node is in the vector
    else
    {
        // Determine socail tie value and place in vector
        s.socialTieValue = ComputeSocialTie (index, time);
        socialTable[index] = s;
    }
}

void Relationship::UpdateAndMergeSocialTable (Peer peer2, Time time, SocialTableEntry *socialTableAddress, uint32_t size)
{
    // Update the table
    UpdateSocialTable (peer2, time);

    // Merge the updated tables
    MergeSocialTables(socialTableAddress, size);

    // Compute the centrality
    ComputeCentrality ();

    // Determine the communitues in the centrality table for the clusters
    DetermineCommunities ();

    // Compute the socail tie level for the centrality table
    // This fails when K is less than or equal to the number of point must ensure that there are more points than K
    for (uint32_t i=0; i < communityListSize; i++)
    {
        GenerateCenters (K_CLUSTER, centralityTable, centralityTableSize, centers, communityList[i].commID);
    }
}

// Create a centrality table for a singel community
void Relationship::BuildCommunityTable (Community comm)
{
    uint32_t index = 0;

    for (int i = 0; i < comm.nodeNum; i++)
    {
        if (centralityTable[i].node.commID == comm.commID)
        {
           communityCentralityTable[index] = &centralityTable[i];
            index++;
        }
    }
}

double Relationship::GetSocialTie (Ipv4Address peer1, Ipv4Address peer2) const
{
    int index = -1;

    for (uint16_t i=0; i < socialTableSize; i++)
    {
        if ((peer1 == socialTable[i].peer1.ID) && (peer2 == socialTable[i].peer2.ID))
        {
            index = i;
            break;
        }
    }

    if (index < 0)
        return 0;
    else
        return socialTable[index].socialTieValue;
}

SocialTableEntry* Relationship::GetSocialTableAddress()
{
    return socialTable;
}

CentralityTableEntry *Relationship::GetCentralityTableAddress (uint32_t &size)
{
    size = centralityTableSize;

    return centralityTable;
}

int *Relationship::GetCentralityArrayAddress (int &size)
{
    for (uint32_t i=0; i < centralityTableSize; i++)
    {
        centralityArray[i] = centralityTable[i].centrality * 1000;
    }

    size = centralityTableSize;

    return centralityArray;
}

uint32_t Relationship::GetCentralityTableSize () const
{
    return centralityTableSize;
}

void Relationship::DetermineCommunities ()
{
    uint32_t index;
    communityListSize = 0;

    // Cycle through the centrality table to place all the unique communites in a list
    for (uint32_t i = 0; i < centralityTableSize; i++)
    {
        for (index = 0; index < communityListSize; index++)
        {
            if (centralityTable[i].node.commID == communityList[index].commID)
                break;
        }

        // If index is equal to the size of the list then the community is not a part of
        // the list and needs to be addedd
        if (index == communityListSize)
        {
            communityList[index].commID = centralityTable[i].node.commID;
            communityListSize++;
        }
    }

    // Determine the number of nodes in community
    for (uint32_t i = 0; i < communityListSize; i++)
    {
        communityList[i].nodeNum = UniqueNodeCount(communityList[i].commID);
    }

}

void Relationship::MergeSocialTables (SocialTableEntry *peerTable, uint32_t size)
{
    uint16_t j;
    uint16_t initSize = socialTableSize;

    for (uint32_t i=0; i < size; i++)
    {
        for(j=0; j < initSize; j++)
        {
            // Search for mathcing entry in both peerTables
            if ((socialTable[j].peer1.ID == (peerTable+i)->peer1.ID) && (socialTable[j].peer2.ID == (peerTable+i)->peer2.ID))
            {
                // Compare timestamps
                if ((peerTable+i)->timestamp > socialTable[j].timestamp)
                {
                    // Replace if peer peerTable is more recent
                    socialTable[j] = *(peerTable+i);
                }
                break;
            }

            //TODO try and take this out of the loop
        }
        // If no previous match found add to social peerTable
        if (j == initSize)
        {
            socialTable[socialTableSize] = *(peerTable+i);
            socialTableSize = (socialTableSize + 1) % ARRAY_SIZE;
        }

    }
}


uint32_t Relationship::UniqueNodeCount (uint32_t commID)
{
    uint32_t localTableSize = 0; // Start table over every time centrality is computed
    uint32_t index;

    Ipv4Address *localTable = new Ipv4Address[socialTableSize];

    // Iterate through the social table and determine if node already been counted
    for (uint32_t i=0; i < socialTableSize; i++)
    {
        // Determine if node ID has been counted already
	    for (index = 0; index < localTableSize; index++)
        {
            if (socialTable[i].peer1.ID == localTable[index])
                break;
        }

       if (index == localTableSize)
       {
            if ((socialTable[i].peer1.commID == commID) && 
				(socialTable[i].peer2.commID == commID))
			{
            	localTable[index] = socialTable[i].peer1.ID;
            	localTableSize++;
			}
       }
    }

    delete [] localTable;
    return localTableSize;
}


/* Old function counts all the nodes in social table
uint32_t Relationship::UniqueNodeCount ()
{
    uint32_t localTableSize = 0; // Start table over every time centrality is computed
    uint32_t index;

    Ipv4Address *localTable = new Ipv4Address[socialTableSize];

    // Iterate through the social table and construct the centrality table
    for (uint32_t i=0; i < socialTableSize; i++)
    {
        // Look for matching node IDs in both tables
	    for (index = 0; index < localTableSize; index++)
        {
            if (socialTable[i].peer1.ID == localTable[index])
                break;
        }

       if (index == localTableSize)
       {
            localTable[index] = socialTable[i].peer1.ID;
            localTableSize++;
       }
    }

    delete [] localTable;
    return localTableSize;

}


// Old function computes centrality over all nodes in social table
void Relationship::ComputeCentrality ()
{    
    centralityTableSize = 0; // Start table over every time centrality is computed
    double alpha = 0.5; // 
    uint32_t index;
    CentralityTableEntry c;

    uint32_t nodeCount = UniqueNodeCount ();

    // Iterate through the social table and construct the centrality table
    for (uint32_t i=0; i < socialTableSize; i++)
    {
        // Look for matching node IDs in both tables
        // If ID found then the node doesn't need to be added to centrality table
	  
        {
            if (socialTable[i].peer1.ID == centralityTable[index].node.ID)
                break;
        }

        // Insert centrality entry at index and increase centrality table size
        if (index == centralityTableSize)
        {
            double RkSum = 0; // social tie sum
            double RkSquSum = 0; // social tie sqared sum
            //uint16_t n = 0; // Number of nodes observed
            
            // Sum up the Rk values and increment n
            for (uint32_t j=i; j < socialTableSize; j++)
            {
                // Compare the id of candidate to every other node in table
                if (socialTable[i].peer1.ID == socialTable[j].peer1.ID)
                {
                    RkSum = RkSum + socialTable[j].socialTieValue;
                    RkSquSum = RkSquSum + pow(socialTable[j].socialTieValue,2);
                    //n++;
                }
            }
             
            double equPt1;
            double equPt2;
            // First part of the equation
            //equPt1 = alpha * (RkSum)/n;
            equPt1 = alpha * (RkSum)/nodeCount;

            // Second part of the equation
            //equPt2 = (1 - alpha) * pow(RkSum,2) / (n * RkSquSum);
            equPt2 = (1 - alpha) * pow(RkSum,2) / (nodeCount * RkSquSum);
            
            // Insert Node ID into centrality entry
            c.node = socialTable[i].peer1;
            // Compute centrality for entry
            c.centrality = equPt1 + equPt2;
            
            centralityTable[index] = c; // Insert centrality entry
            centralityTableSize++; // Increment the size of of the centrality table because entry was made
        }
    }

}

// OLD funtion no longer used
CentralityTableEntry *Relationship::GetHigherCentralityNodes (uint32_t &size)
{
    double centrality = -1;
    size = 0;
    // Find the nodes centrality
    for (uint32_t i = 0; i < centralityTableSize; i++)
    {
        if (nodeID == centralityTable[i].nodeID)
        {
            centrality = centralityTable[i].centrality;
            break;
        }
    }

    if (centrality > 0)
    {
        for (uint32_t i=0; i < centralityTableSize; i++)
        {
            if (centrality > centralityTable[i].centrality)
            {
                higherCentralityTable[i] = centralityTable[i];
                size++;
            }
        }
    }
    // Return the beginning of the table
    return  higherCentralityTable;
}
*/

void Relationship::ComputeCentrality ()
{    
    centralityTableSize = 0; // Start table over every time centrality is computed
    //double alpha = 0.5; // 
    uint32_t index;
    CentralityTableEntry c;
 
    // Iterate through the social table and construct the centrality table
    for (uint32_t i=0; i < socialTableSize; i++)
    {
        // Look for matching node IDs in centrality table
        // If ID found then the node doesn't need to be added to centrality table because it already exist
        for (index = 0; index < centralityTableSize; index++)
        {
            if ((socialTable[i].peer1.ID == centralityTable[index].node.ID) || 
				(socialTable[i].peer1.commID != socialTable[i].peer2.commID))
                break;
        }

        // Insert centrality entry at index and increase centrality table size
        if (index == centralityTableSize)
        {
            // Count the unique occurences of peer encounters w/in community
            //uint32_t nodeCount = UniqueNodeCount (socialTable[i].peer1.commID);
            double RkSum = 0; // social tie sum
            double RkSquSum = 0; // social tie sqared sum
			//uint16_t n = 0; // Number of nodes observed
            // Sum up the Rk values and increment n
            for (uint32_t j=i; j < socialTableSize; j++)
            {
                // Compare the id of candidate to every other node in table
                // and check if peer node is in the same community
                if ((socialTable[i].peer1.ID == socialTable[j].peer1.ID) && 
                        (socialTable[i].peer1.commID == socialTable[j].peer2.commID))
		{
                    RkSum = RkSum + socialTable[j].socialTieValue;
                    RkSquSum = RkSquSum + pow(socialTable[j].socialTieValue,2);
                    //n++;
                }
            }
			
            if (RkSum > 0)
            {
                //double equPt1;
                //double equPt2;
		// First part of the equation
		// equPt1 = alpha * (RkSum)/n;
		//equPt1 = alpha * (RkSum)/nodeCount;

		// Second part of the equation
		//equPt2 = (1 - alpha) * pow(RkSum,2) / (n * RkSquSum);
		//equPt2 = (1 - alpha) * pow(RkSum,2) / (nodeCount * RkSquSum);
				
		// Insert Node ID into centrality entry
		c.node = socialTable[i].peer1;
		// Compute centrality for entry
		//c.centrality = equPt1 + equPt2;
                c.centrality = pow(RkSum, 3)/RkSquSum;
				
		centralityTable[index] = c; // Insert centrality entry
		centralityTableSize++; // Increment the size of of the centrality table because entry was made
            }
            else
            {
                // Insert Node ID into centrality entry
                c.node = socialTable[i].peer1;
                // Compute centrality for entry
                c.centrality = 0;
                
                centralityTable[index] = c; // Insert centrality entry
                centralityTableSize++; // Increment the size of of the centrality table because entry was made
            }
        }
    }

}

Ipv4Address *Relationship::GetHigherCentralityNodes (uint32_t &size)
{
    double centrality = -1;
    size = 0;
    // Find the nodes centrality
    for (uint32_t i = 0; i < centralityTableSize; i++)
    {
        if (me.ID == centralityTable[i].node.ID)
        {
            centrality = centralityTable[i].centrality;
            break;
        }
    }

    if (centrality > 0)
    {
        for (uint32_t i=0; i < centralityTableSize; i++)
        {
            if (centrality < centralityTable[i].centrality)
            {
                higherCentralityTable[size] = centralityTable[i].node.ID;
                size++;
            }
        }
    }
    // Return the beginning of the table
    return  higherCentralityTable;
}

void Relationship::PrintSocialTable ()
{

    if (socialTableSize == 0)
    {
        std::cout << "There are no Social Ties for this node\n";
    }
    else
    {
        std::cout << "Peer1\t\t" << "CommID1\t" << "Peer2\t\t" << "CommID2\t" << "Time\t" << "SocialTie" << std::endl;

        for (uint32_t i=0; i < socialTableSize; i++)
        {
            std::cout << socialTable[i].peer1.ID << "\t\t" << socialTable[i].peer1.commID << "\t" << socialTable[i].peer2.ID 
                << "\t\t" << socialTable[i].peer2.commID << "\t" << socialTable[i].timestamp.GetSeconds() 
                << "\t"  << socialTable[i].socialTieValue << std::endl;
        }
    }
}

void Relationship::PrintCentralityTable ()
{
    if (centralityTableSize == 0)
    {
        std::cout << "This node has no Centrality values\n";
    }
    else
    {
        std::cout << "NodeID\t\t" << "CommID\t\t" << "Centrality" << std::endl;

        for (uint32_t i=0; i < centralityTableSize; i++)
        {
            std::cout << centralityTable[i].node.ID << "\t\t" << centralityTable[i].node.commID << "\t\t" 
                << centralityTable[i].centrality << std::endl;
        }
    }
}

void Relationship::PrintSocialLevelTable ()
{
    if (centralityTableSize == 0)
    {
        std::cout << "This node has no Social Level Values values\n";
    }
    else
    {
        std::cout << "NodeID\t\t" << "SocialLevel" << std::endl;

        for (uint32_t i=0; i < centralityTableSize; i++)
        {
            std::cout << centralityTable[i].node.ID << "\t\t" << centralityTable[i].SocialLevel << std::endl;
        }
    }
}

void Relationship::PrintCommunityList ()
{
    if (communityListSize == 0)
        std::cout << "This node has no community list\n";
    else
    {
        std::cout << "CommID\t" << "# of Nodes\n";

        for (uint32_t i = 0; i < communityListSize; i++)
        {
            std::cout << communityList[i].commID << "\t" << communityList[i].nodeNum << std::endl;
        }
    }
}

Ipv4Address Relationship::GetHigherSocialTie (Ipv4Address peer1, Ipv4Address peer2, Ipv4Address destination)
{
    double peer1SocialTie = 0;
    double peer2SocialTie = 0;

    Ipv4Address dummyAddress = "0.0.0.0"; // Returned if no node has higher value

    for (uint32_t index = 0; index < socialTableSize; index++)
    {
        // Check if the peers are in the social table
        if ((peer1 == socialTable[index].peer1.ID) && (destination == socialTable[index].peer2.ID))
        {
            peer1SocialTie = socialTable[index].socialTieValue;
        }
        
        else if ((peer2 == socialTable[index].peer1.ID) && (destination == socialTable[index].peer2.ID))
        {
            peer2SocialTie = socialTable[index].socialTieValue;
        }

        // End loop when a value is set for both peers
        if ((peer1SocialTie > 0) && (peer2SocialTie > 0))
        {
            break;
        }
    }

     // If the peers are equal return the dummy address
    if (peer1SocialTie == peer2SocialTie)
    {
        return dummyAddress;
    }
    else if (peer2SocialTie > 1 * peer1SocialTie)
    {
        return peer2;
    }
    else
    {
        return peer1;
    }

}

Ipv4Address Relationship::GetHigherSocialLevel (Ipv4Address peer1, Ipv4Address peer2)
{
    double peer1SocialLevel = 0;
    double peer2SocialLevel = 0;

    Ipv4Address dummyAddress = "0.0.0.0"; // Returned if no node has higher value


    for (uint32_t i = 0; i < centralityTableSize; i++)
    {
        // Find the Social level of the peers
        if (peer1 == centralityTable[i].node.ID)
        {
            peer1SocialLevel = centralityTable[i].SocialLevel;
        }
        if (peer2 == centralityTable[i].node.ID)
        {
            peer2SocialLevel = centralityTable[i].SocialLevel;
        }
        // End loop if a level is found for both peers
        if ((peer1SocialLevel > 0) && (peer2SocialLevel > 0))
        {
            break;
        }
    }

    // If the peers are equal return the dummy address
    if (peer1SocialLevel == peer2SocialLevel)
    {
        return dummyAddress;
    }
    else if (peer2SocialLevel > 1 * peer1SocialLevel)
    {
        return peer2;
    }
    else
    {
        return peer1;
    }
}

bool Relationship::PeerCounted (BorderNode bn [], Ipv4Address node, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        if (bn[i].ID == node)
            return true;
    }
    return false;
}

bool Relationship::PeerCompared (Ipv4Address pc [], Ipv4Address node, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        if (pc[i] == node)
            return true;
    }
    return false;
}


Ipv4Address Relationship::GetBestBorderNode (uint32_t commID)
{
    Ipv4Address dummyAddress = "0.0.0.0";
    BorderNode *nodeCountArray = new BorderNode[socialTableSize];
    uint32_t nodeCountArraySize = 0;
    BorderNode highest = {dummyAddress, 0};
    uint32_t count = 0;
    Ipv4Address tmpPeer, cmpPeer;

    for (uint32_t i = 0; i < socialTableSize; i++) // Iterate through the socialTable
    {
        // Keep track of peers compared for this node
        Ipv4Address *peersComparedArray = new Ipv4Address[socialTableSize];
        uint32_t peersComparedArraySize = 0;

        // Check if the peers in table enrty match the community of the node that called the function
        // and the community of interest. If not continue to the next entry
        if ((socialTable[i].peer1.commID == me.commID) && (socialTable[i].peer2.commID == commID))
        {
            tmpPeer = socialTable[i].peer1.ID;
            cmpPeer = socialTable[i].peer2.ID;
        }
        else if ((socialTable[i].peer1.commID == commID) && (socialTable[i].peer2.commID == me.commID))
        {
            tmpPeer = socialTable[i].peer2.ID;
            cmpPeer = socialTable[i].peer1.ID;
        }
        else
        {
            continue;
        }
        
        // Check if the node has been counted if so go to the next entry 
        if (PeerCounted(nodeCountArray, tmpPeer, nodeCountArraySize))
        {
            continue;
        }
        else
        {
            // Count this entry and add node that was just compared to peersComaparedArray
            count = 1;
            peersComparedArray[peersComparedArraySize] = cmpPeer;
            peersComparedArraySize++;
                       
            // Iterate throught rest of table count border encounters
            for (uint32_t j=i+1; j < socialTableSize; j++)
            {
                if ((tmpPeer == socialTable[j].peer1.ID) && (socialTable[j].peer2.commID == commID))
                {
                    if (PeerCompared(peersComparedArray, socialTable[j].peer2.ID, peersComparedArraySize))
                        continue;
                    else
                        count++;
                        peersComparedArray[peersComparedArraySize] = socialTable[j].peer2.ID;
                        peersComparedArraySize++;
                }

                else  if ((tmpPeer == socialTable[j].peer2.ID) && (socialTable[j].peer1.commID == commID))
                
                {
                    if (PeerCompared(peersComparedArray, socialTable[j].peer1.ID, peersComparedArraySize))
                        continue;
                    else
                        count ++;
                        peersComparedArray[peersComparedArraySize] = socialTable[j].peer1.ID;
                        peersComparedArraySize++;

                }
                
                
            }

            // Determine if the peer just counted is higher then any of the previous counts
            if (count > highest.count)
            {
                highest.ID = tmpPeer;
                highest.count = count;
            }
            
            // Add the node just counted to the nodeCounted Array
            nodeCountArray[nodeCountArraySize].ID = tmpPeer;
            nodeCountArray[nodeCountArraySize].count = count;
            nodeCountArraySize++;
        }

        delete [] peersComparedArray;

    }

    delete [] nodeCountArray;

    return highest.ID;
}

Ipv4Address Relationship::GetHigherCentralityNode (Ipv4Address peer1, Ipv4Address peer2)
{
    double peer1Centrality = 0;
    double peer2Centrality = 0;

    Ipv4Address dummyAddress = "0.0.0.0"; // Returned if no node has higher value


    for (uint32_t i = 0; i < centralityTableSize; i++)
    {
        // Find the Social level of the peers
        if (peer1 == centralityTable[i].node.ID)
        {
            peer1Centrality = centralityTable[i].centrality;
        }
        if (peer2 == centralityTable[i].node.ID)
        {
            peer2Centrality = centralityTable[i].centrality;
        }
        // End loop if a level is found for both peers
        if ((peer1Centrality > 0) && (peer2Centrality > 0))
        {
            break;
        }
    }

    // If the peers are equal return the dummy address
    if (peer1Centrality == peer2Centrality)
    {
        return dummyAddress;
    }
    else if (peer2Centrality > 1 * peer1Centrality)
    {
        return peer2;
    }
    else
    {
        return peer1;
    }

}

bool Relationship::HasHighestSocialLevel (Ipv4Address targetNodeID, uint32_t commID)
{
	uint32_t index;
	int nodeSocialLevel = 0;

	// Check if a node matching target node and community exist in the centrality table
	for (index = 0; index < centralityTableSize; index++)
	{
		if ((centralityTable[index].node.ID == targetNodeID)
			&& (centralityTable[index].node.commID == commID))
		{
			nodeSocialLevel = centralityTable[index].SocialLevel;
			break;
		}
	}

	// If the no match found return dummyID
	if (index == centralityTableSize)
	{
		return false;
	}
	// If match found determind if traget node is the highest in it's community
	else
	{
		for (uint32_t i = 0; i < centralityTableSize; i++)
		{
			// If there exist a node in the table with a higher social level than the target node then return false
			if ((centralityTable[i].SocialLevel > nodeSocialLevel) && (targetNodeID != centralityTable[i].node.ID))
			{
				return false;
			}
		}

		// If iterate though the entire table and a higher node isn't found then the target node has the highest social level
		return true;
	}
}

} // namespace ns3
