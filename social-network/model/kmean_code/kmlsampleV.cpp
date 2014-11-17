
#include <cstdlib>			// C standard includes
#include <iostream>			// C++ I/O
#include <string>			// C++ strings
#include "KMlocal.h"			// k-means algorithms
#include <algorithm>
#include "kmlsample.h"

using namespace std;			// make std:: available

namespace ns3
{


int	stages		= 100;		// number of stages

//----------------------------------------------------------------------
//  Termination conditions
//	These are explained in the file KMterm.h and KMlocal.h.  Unless
//	you are into fine tuning, don't worry about changing these.
//----------------------------------------------------------------------
KMterm	term(100, 0, 0, 0,		// run for 100 stages
		0.10,			// min consec RDL
		0.10,			// min accum RDL
		3,			// max run stages
		0.50,			// init. prob. of acceptance
		10,			// temp. run length
		0.95);			// temp. reduction factor


//extern  CentralityTableEntry * myList;


/*
Returns length
*/
int ListNodeWithHigherSocialLevel(int NodeSocialLevel, Ipv4Address *NodeList,  CentralityTableEntry * myList, int ListLength)
{
	int index = 0;
	for(int i = 0; i < ListLength; i++)
	{

		if(myList[i].SocialLevel > NodeSocialLevel)
		{
			NodeList[index] = myList[i].node.ID;
			index++;
		}
	}

	return index;
}


int AssignGroup(int num1, double * centers, int NumCenters)
{
	int group1 = 0;
	for(int i = 0; i < NumCenters; i++)
	{
		double threshold = centers[i];
		if (num1 > threshold)
		{
			group1++;
		}
	}

	return group1;

}



void GenerateCenters(int k, CentralityTableEntry ** myList, int ValueLength, double * centers)
{
	term.setAbsMaxTotStage(stages);		// set number of stages
    KMdata dataPts(1, ValueLength);		// allocate data storage
	KMpointArray pts = dataPts.getPts();


	for (int i = 0; i < ValueLength; i++) 		
	{
		int centrality = (KMcoord) (myList[i]->centrality);
		pts[i][0] = centrality;	
	}
    
	dataPts.setNPts(ValueLength);			// set actual number of pts
    dataPts.buildKcTree();			// build filtering structure

    KMfilterCenters ctrs(k, dataPts);		// allocate centers
    				
    //cout << "\nExecuting Clustering Algorithm: Lloyd's\n";
    KMlocalLloyds kmLloyds(ctrs, term);		// repeated Lloyd's
    ctrs = kmLloyds.execute();			// execute

	KMctrIdxArray closeCtr = new KMctrIdx[dataPts.getNPts()];
    double* sqDist = new double[dataPts.getNPts()];
    ctrs.getAssignments(closeCtr, sqDist);

	KMcenterArray cpts1 = ctrs.getCtrPts();
	KMpoint p1 = *cpts1;

	for(int i = 0; i < k; i++)
	{
		double c1 = *p1;
		double c1_i = (double)c1;
		centers[i] = c1_i;
		p1++;
	}

	std::sort(centers,centers+k);

	
	

	/*
	Assign social levels based on centrality
	*/
	for(int i = 0; i < ValueLength; i++)
	{
		double centr = myList[i]->centrality;
		int group = AssignGroup(centr,centers,k);
		myList[i]->SocialLevel = group;
	}


	delete [] closeCtr;
    delete [] sqDist;


	return;

}

//----------------------------------------------------------------------
//  Main program
//----------------------------------------------------------------------
/*
int main(int argc, char **argv)
{

	// Creating an input testcase 

	int * centers  = new int[5];
	int * nodelist = new int[100];

	memset(nodelist,0,100);

	CentralityTableEntry *MyList = new NodeStructure[100];

	for(int i = 0; i < 100; i++)
	{
		MyList[i].centrality=i;
		MyList[i].nodeId = i;
	}

	
	// Do the K-clustering and generate the centers
	

	GenerateCenters(5, MyList, 100, centers);

	 
	int a = centers[0];
	int b = centers[1];:w

	int c = centers[2];
	int d = centers[3];
	int e = centers[4];

	int len = ListNodeWithHigherSocialLevel(4, nodelist, MyList, 100);

	for(int i = 0; i < len; i++)
	{
		cout << nodelist[i] << "\n";
	}


	 delete [] MyList;
	 delete [] nodelist;
	 delete [] centers;


	//AssignSocialLevels(

}
*/
} // ns3 namespace
