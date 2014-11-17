#include "ns3/log.h"
#include "ns3/content-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ContentManager");

ContentManager::ContentManager ()
{
    m_size = 0;
    m_array = new ContentOwnerEntry[1000];
}

ContentManager::~ContentManager ()
{
    delete [] m_array;
}

void
ContentManager::Insert(ContentOwnerEntry content)
{
    if (m_size == 1000)
    {
        return;
    }
    
    m_array[m_size] = content;
    m_size++;
}

ContentOwnerEntry *
ContentManager::GetContentArray() const
{
    return m_array;
}

uint32_t
ContentManager::GetContentArraySize() const
{
    return m_size;
}

void
ContentManager::Merge(ContentOwnerEntry *anotherArray, uint32_t anotherArraySize)
// assume anotherArray has unique elements
{   
    for (uint32_t i=0; i<anotherArraySize; ++i)
    {
        if ( !Exist(anotherArray[i]) ) 
        {
            Insert(anotherArray[i]);
        }
    }
}

bool
ContentManager::Exist(ContentOwnerEntry content) const
{
    for (uint32_t i=0; i<m_size; ++i)
    {
        if ( (m_array[i].contentName.IsEqual(content.contentName)) &&
             (m_array[i].contentOwner.IsEqual(content.contentOwner)) )
        {
            return true;
        }
    }
    return false;
}

Ipv4Address
ContentManager::GetOwnerOfContent(Ipv4Address content) const
{
    for (uint32_t i=0; i<m_size; ++i)
    {
        if (m_array[i].contentName.IsEqual(content))
        {
            return m_array[i].contentOwner;
        }
    }
    return Ipv4Address("0.0.0.0");
}


}
