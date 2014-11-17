
#ifndef CONTENT_MANAGER_H
#define CONTENT_MANAGER_H

#include "ns3/ipv4-address.h"

//#define MAX_SIZE 1000 =>cause a compiler error

using namespace std;

namespace ns3
{

struct ContentOwnerEntry
{
    ContentOwnerEntry() {}
    ContentOwnerEntry(Ipv4Address contentOwner, Ipv4Address contentName, uint32_t communityId)
    {
        this->contentOwner = contentOwner;
        this->contentName = contentName;
        this->communityId = communityId;
    }
        
    Ipv4Address contentOwner;
    Ipv4Address contentName;
    uint32_t communityId; //communityId of content owner
};

class ContentManager
{
public:    
    ContentManager ();
    virtual ~ContentManager ();
    
    void Insert(ContentOwnerEntry entry);
    ContentOwnerEntry *GetContentArray() const;
    uint32_t GetContentArraySize() const;
    
    //check if content exists in this table
    //must match both the contentOwner and contentName
    bool Exist(ContentOwnerEntry content) const;
    
    //only merge element that does not yet exist in m_array
    void Merge(ContentOwnerEntry *anotherArray, uint32_t anotherArraySize); //merge another_array into m_array
    
    //return the owner of the content.
    //Return Ipv4Address("0.0.0.0") if the content does not exist.
    Ipv4Address GetOwnerOfContent(Ipv4Address content) const;
    
    //return 0 if not found
    uint32_t GetCommunityIdOfOwnerOfContent(Ipv4Address contentOwner) const;
    
private:
    ContentOwnerEntry *m_array; //contain only unique entry
    uint32_t m_size;

};

}	// namespace ns3

#endif /* CONTENT_MANAGER_H */
