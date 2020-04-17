#include <iostream>
#include <climits>
#include <cmath>
#include <fstream>
#include <cassert>

using namespace std;

//terminate due to error;
void abort()
{
	cout << "usage: ./cache-sim FILE_WITH_CACHE_INFO FILE_OF_RESULTS" << endl;
	exit(EXIT_FAILURE);
}

//reset ifstream object to beginning of file;
void resetIfstream(ifstream& f)
{
	//reset eof bit;
	if(f.eof())
		f.clear();
	f.seekg(0, f.beg);
}

//output results to console and reset cache hits and cache accesses;
void outputAndReset(int& h,int& a)
{
	cout << h << "," << a << ";";
	h = a = 0;
}

//print results to text file;
void printToFile(ofstream& f, int h, int a)
{
	string output;
	f << h << "," << a << ";";
}

//returns true if tag is found in the cache set and updates tag to be MRU element;
bool find(int *set, int tag, int numOfWays)
{
	for(int i = 0; i < numOfWays; i++)
		//if found;
		if(set[i] == tag)
		{
			int lastPos = numOfWays - 1;
			for(int j = i; j < numOfWays - 1; j++)
			{
				//if found and set still has empty lines;
				//replace last non empty element;
				if(set[j + 1] == -1)
				{
					lastPos = j;
					break;
				}
				set[j] = set[j + 1];
			}
			set[lastPos] = tag;
			return true;
		}
	return false;
}

//inserts tag into the cache set (using LRU policy) and returns the replaced value;
//LRU = leftmost element, MRU = rightmost element;
int insert(int *set, int tag, int numOfWays)
{
	int temp = set[0];
	for(int i = 0; i < numOfWays; i++)
	{
		//if cache set still has empty lines, put tag in next empty line;
		if(set[i] == -1)
		{
			set[i] = tag;
			return temp;
		}
		//if cache set is full, shift everything left and put tag at end;
		else if(i == numOfWays - 1)
		{
			for(int j = 0; j < numOfWays - 1; j++)
				set[j] = set[j + 1];
			set[i] = tag;
			return temp;
		}
	}
	return temp;
}

//sets the hit rate for an n-way set associative cache;
//utilizes one of the following replacement policies:
//		LRU		(replaces the least recently used element, on both hits and misses)
//		PLRU	(pseudo LRU, approximates LRU policy using hot-cold bits or hot-cold tree)
//		noAllocationOnWriteMiss (if a store ("S") instruction misses, write directly into memory instead of the cache)
//		prefetchNextLine		(gets the tag for the next cache line, both on hits and misses)
//		prefetchNextLineOnMiss	(gets the tag for the next cache line only on cache misses)
void setAssociativeCache(int& h, int& a, ifstream& f, int cacheSize, int lineSize, int numOfWays, string replacementPolicy)
{
	string flag;
	unsigned long int addr, nextAddrTag;
	int numOfSets, tag, set, offset;

	assert(numOfWays >= 1 && "numOfWays cannot be less than 1");
	numOfSets = cacheSize / (lineSize * numOfWays);
	int cache[numOfSets][numOfWays];
	for(int i = 0; i < numOfSets; i++)
		for(int j = 0; j < numOfWays; j++)
			cache[i][j] = -1;
		
	int n = numOfWays + (numOfWays - 1);
	//use complete binary tree array to model hotColdBits;
	//cache set is made up of the leaf nodes;
	int hotColdArr[numOfSets][n];
	for(int i = 0; i < numOfSets; i++)
		for(int j = 0; j < n; j++)
			hotColdArr[i][j] = 0;
/***************************************************/
	if(replacementPolicy == "LRU")
	{
		while(f >> flag >> std::hex >> addr)
		{
			a++;
			offset = addr & (lineSize - 1);
			set = (addr >> (int)log2(lineSize)) & (numOfSets - 1);
			tag = (addr >> ((int)log2(numOfSets) + (int)log2(lineSize))) & (ULONG_MAX);

			if(find(cache[set], tag, numOfWays) == true)
				h++;
			else
				insert(cache[set], tag, numOfWays);
		}
	}
/***************************************************/
	else if(replacementPolicy == "PLRU")
	{
		while(f >> flag >> std::hex >> addr)
		{
			a++;
			offset = addr & (lineSize - 1);
			set = (addr >> (int)log2(lineSize)) & (numOfSets - 1);
			tag = (addr >> ((int)log2(numOfSets) + (int)log2(lineSize))) & (ULONG_MAX);
/*
			for(int i = 0, j = 1, k = 0; i < n; i++, k++)
			{
				if(k == j){cout << endl; j *= 2; k = 0;}
				cout << hotColdArr[set][i] << " ";
			}
			cout << endl << endl;
*/
			for(int i = 0; i <= numOfWays; i++)
			{
				int x = 0;
				//get index of leaf node (cache[set][x]);
				for(int childToGoTo = hotColdArr[set][0]; x < (n - 1) / 2; )
				{
					hotColdArr[set][x] = (hotColdArr[set][x] + 1) & 1;
					//0 means left child is LRU, therefore go to left child;
					if(childToGoTo == 0)
						x = (2 * x) + 1;
					//1 means right child is LRU, therefore go to right child;
					else if(childToGoTo == 1)
						x = (2 * x) + 2;
					childToGoTo = hotColdArr[set][x];
				}

				//if leaf node contains the tag;
				if(hotColdArr[set][x] == tag)
				{
					h++;
					break;
				}
				//if traversed through all leaf nodes and tag's not found;
				else if(i == numOfWays)
					hotColdArr[set][x] = tag;
			}
		}

/*
			for(int i = 0, j = 1, k = 0; i < n; i++, k++)
			{
				if(k == j){cout << endl; j *= 2; k = 0;}
				cout << hotColdArr[set][i] << " ";
			}
			cout << endl << endl;
//*/
	}
/***************************************************/
	else if(replacementPolicy == "noAllocationOnWriteMiss")
	{
		while(f >> flag >> std::hex >> addr)
		{
			a++;
			offset = addr & (lineSize - 1);
			set = (addr >> (int)log2(lineSize)) & (numOfSets - 1);
			tag = (addr >> ((int)log2(numOfSets) + (int)log2(lineSize))) & (ULONG_MAX);

			//LRU replacement policy;			
			if(find(cache[set], tag, numOfWays) == true)
				h++;
			else if(flag != "S")
				insert(cache[set], tag, numOfWays);
			else if(flag == "S")
				{
					//code to write tag directly into memory due to store instruction;
				}
		}
	}
/***************************************************/
	else if(replacementPolicy == "prefetchNextLine")
	{
		while(f >> flag >> std::hex >> addr)
		{
			a++;
			offset = addr & (lineSize - 1);
			set = (addr >> (int)log2(lineSize)) & (numOfSets - 1);
			tag = (addr >> ((int)log2(numOfSets) + (int)log2(lineSize))) & (ULONG_MAX);

			nextAddrTag = ((addr + lineSize) >> ((int)log2(numOfSets) + (int)log2(lineSize))) & ULONG_MAX;

			//uses LRU replacement policy for current and prefetched tag;
			if(find(cache[set], tag, numOfWays) == true)
			{
				h++;
				//if nextAddrTag not found in cache[set], add it;
				if(find(cache[set], nextAddrTag, numOfWays) == false)
					insert(cache[set], nextAddrTag, numOfWays);
			}
			else
			{
				insert(cache[set], tag, numOfWays);
				if(find(cache[set], nextAddrTag, numOfWays) == false)
					insert(cache[set], nextAddrTag, numOfWays);
			}
		}
	}
/***************************************************/
	else if(replacementPolicy == "prefetchNextLineOnMiss")
	{
		while(f >> flag >> std::hex >> addr)
		{
			a++;
			offset = addr & (lineSize - 1);
			set = (addr >> (int)log2(lineSize)) & (numOfSets - 1);
			tag = (addr >> ((int)log2(numOfSets) + (int)log2(lineSize))) & (ULONG_MAX);

			nextAddrTag = ((addr + lineSize) >> ((int)log2(numOfSets) + (int)log2(lineSize))) & ULONG_MAX;

			if(find(cache[set], tag, numOfWays) == true)
				h++;
			else
			{
				insert(cache[set], tag, numOfWays);
				if(find(cache[set], nextAddrTag, numOfWays) == false)
					insert(cache[set], nextAddrTag, numOfWays);
			}
		}
	}
/***************************************************/
}

//sets the hit rate for a direct mapped cache;
void directMappedCache(int& h, int& a, ifstream& f, int cacheSize, int lineSize)
{
	//a directMappedCache is simply a 1-way n set associative cache;
	setAssociativeCache(h, a, f, cacheSize, lineSize, 1, "LRU");
}

//sets the hit rate for a fully associative cache;
void fullyAssociativeCache(int& h, int& a, ifstream& f, int cacheSize, int lineSize, string replacementPolicy)
{
	//a fullyAssociativeCache is simply an n-way 1 set associative cache;
	//cacheSize = numOfSets * lineSize * numOfWays;
	int numOfWays = cacheSize / lineSize;
	setAssociativeCache(h, a, f, cacheSize, lineSize, numOfWays, replacementPolicy);
}

//driver function to test different kinds of caches;
int main(int argc, char *argv[]) 
{
	int hits = 0;
	int accesses = 0;

	ifstream inFile(argv[1]);
	if(!inFile.is_open())
		abort();

	ofstream outFile(argv[2]);
	if(!outFile.is_open())
		abort();

/*
	directMappedCache(hits, accesses, inFile, 1024, 32);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	directMappedCache(hits, accesses, inFile, 4096, 32);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	directMappedCache(hits, accesses, inFile, 16384, 32);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	directMappedCache(hits, accesses, inFile, 32768, 32);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;

	setAssociativeCache(hits, accesses, inFile, 16384, 32, 2, "LRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 4, "LRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 8, "LRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 16, "LRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;

	fullyAssociativeCache(hits, accesses, inFile, 16384, 32, "LRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
*/

//WRONG---------------------------
	fullyAssociativeCache(hits, accesses, inFile, 16384, 32, "PLRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
//WRONG---------------------------

/*
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 2, "noAllocationOnWriteMiss");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 4, "noAllocationOnWriteMiss");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 8, "noAllocationOnWriteMiss");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 16, "noAllocationOnWriteMiss");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
//*/
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 2, "prefetchNextLine");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 4, "prefetchNextLine");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 8, "prefetchNextLine");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 16, "prefetchNextLine");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;

	setAssociativeCache(hits, accesses, inFile, 16384, 32, 2, "prefetchNextLineOnMiss");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 4, "prefetchNextLineOnMiss");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 8, "prefetchNextLineOnMiss");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 16, "prefetchNextLineOnMiss");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;

	inFile.close();
	outFile.close();
	return EXIT_SUCCESS;
}
