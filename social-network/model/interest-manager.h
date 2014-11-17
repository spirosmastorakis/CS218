
#ifndef INTEREST_MANAGER_H
#define INTEREST_MANAGER_H

#include "ns3/ipv4-address.h"

using namespace std;

namespace ns3
{

struct InterestEntry
{
    InterestEntry() {}
    InterestEntry(Ipv4Address requesterId, uint32_t broadcastId, Ipv4Address content)
    {
        this->requesterId = requesterId;
        this->broadcastId = broadcastId;
        this->requestedContent = content;
    }
    
    Ipv4Address requesterId;
    uint32_t broadcastId;
    Ipv4Address requestedContent;
};

class InterestManager
{
public:    
    InterestManager ();
    virtual ~InterestManager ();
    
    void Insert(InterestEntry entry);
    uint32_t GetInterestArraySize() const;
    
    //return true if the entry already exists
    //return false, otherwise
    bool Exist(InterestEntry entry) const;
   
    
    
private:
    InterestEntry *m_array; //all elements are unique
    uint32_t m_size;

};

}	// namespace ns3

#endif /* INTEREST_MANAGER_H */
