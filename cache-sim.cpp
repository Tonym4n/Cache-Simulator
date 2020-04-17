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
	resetIfstream(f);
	string flag;
	unsigned long int addr, nextAddr;
	int numOfSets, tag, set, offset;
	int nextAddrTag, nextAddrSet, nextAddrOffset;

	assert(numOfWays >= 1 && "numOfWays cannot be less than 1");
	numOfSets = cacheSize / (lineSize * numOfWays);
	int cache[numOfSets][numOfWays];
	for(int i = 0; i < numOfSets; i++)
		for(int j = 0; j < numOfWays; j++)
			cache[i][j] = -1;
		
	int n = numOfWays - 1;
	//use complete binary tree array to model hotColdBits;
	//each element in last level [(n-1)/2]..(n-1) correspond to 2 consecutive elements in cache[set];
	//0 in hotColdArr means left child is hot (MRU) while right is cold (LRU);
	//1 in hotColdArr means right child is hot while left is cold;
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
			
			//j = index of first leaf node, k = cache[set] index;
			int j = (n - 1) / 2, k = 0;
			for(int i = 0; i < numOfWays; i++)
			{
				//if tag is found, set tree path to cache[set][i] as MRU;
				if(cache[set][i] == tag)
				{
					h++;
        			while(k < i)
        			{
          				k++;
          				if(k % 2 == 0)
            				j++;
        			}
        			//set leaf node path;
        			if(k % 2 == 0)
          				hotColdArr[set][j] = 0;
        			else
          				hotColdArr[set][j] = 1;
          			//set node path, going from the bottom level to root;
        			while(true)
        			{
          				bool isEven;
          				if(j % 2 == 0)
            				isEven = true;
          				else
            				isEven = false;
            			//become parent;
          				j = (j - 1) / 2;
          				if(isEven)
            				hotColdArr[set][j] = 1;
          				else
            				hotColdArr[set][j] = 0;
          				if(j == 0)
            				break;
        			}
					break;
				}
				//if tag not found, traverse LRU path and insert;
				else if(i == numOfWays - 1)
				{
					//x = index of leaf node after having traversed LRU path;
					int x = 0;
					while(x < (n - 1) / 2)
					{
						if(hotColdArr[set][x] == 0)
						{
							hotColdArr[set][x] = (hotColdArr[set][x] + 1) & 1;
							x = (2 * x) + 2;
						}
						else if(hotColdArr[set][x] == 1)
						{
							hotColdArr[set][x] = (hotColdArr[set][x] + 1) & 1;
							x = (2 * x) + 1;
						}
					}
					while(j < x) 
					{
						j++; 
						k += 2;
					}
					if(hotColdArr[set][j] == 0)
						cache[set][k + 1] = tag;
					else if(hotColdArr[set][j] == 1)
						cache[set][k] = tag;
					hotColdArr[set][j] = (hotColdArr[set][j] + 1) & 1;
					break;
				}
			}
		}
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

			nextAddr = addr + lineSize;
			nextAddrOffset = nextAddr & (lineSize - 1);
			nextAddrSet = (nextAddr >> (int)log2(lineSize)) & (numOfSets - 1);
			nextAddrTag = (nextAddr >> ((int)log2(numOfSets) + (int)log2(lineSize))) & ULONG_MAX;

			//uses LRU replacement policy for current and prefetched tag;
			if(find(cache[set], tag, numOfWays) == true)
			{
				h++;
				//if nextAddrTag not found in cache[nextAddrSet], insert it;
				if(find(cache[nextAddrSet], nextAddrTag, numOfWays) == false)
					insert(cache[nextAddrSet], nextAddrTag, numOfWays);
			}
			else
			{
				insert(cache[set], tag, numOfWays);
				if(find(cache[nextAddrSet], nextAddrTag, numOfWays) == false)
					insert(cache[nextAddrSet], nextAddrTag, numOfWays);
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

			nextAddr = addr + lineSize;
			nextAddrOffset = nextAddr & (lineSize - 1);
			nextAddrSet = (nextAddr >> (int)log2(lineSize)) & (numOfSets - 1);
			nextAddrTag = (nextAddr >> ((int)log2(numOfSets) + (int)log2(lineSize))) & ULONG_MAX;

			if(find(cache[set], tag, numOfWays) == true)
				h++;
			else
			{
				insert(cache[set], tag, numOfWays);
				if(find(cache[nextAddrSet], nextAddrTag, numOfWays) == false)
					insert(cache[nextAddrSet], nextAddrTag, numOfWays);
			}
		}
	}
/***************************************************/
	else
	{
		cout << "replacementPolicy not found. " << endl;
		exit(EXIT_FAILURE);
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

	cout << "# cacheHits,# cacheAccesses;" << endl;

cout << "Direct Mapped Cache (1KB, 4KB, 16KB, 32KB) w/ line size of 32B" << endl << " ";
	directMappedCache(hits, accesses, inFile, 1024, 32);
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	directMappedCache(hits, accesses, inFile, 4096, 32);
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	directMappedCache(hits, accesses, inFile, 16384, 32);
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	directMappedCache(hits, accesses, inFile, 32768, 32);
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << endl;
	outFile << endl;

cout << "2-, 4-, 8-, and 16- Way Set Associative Cache (16KB) w/ line size of 32B [LRU]" << endl << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 2, "LRU");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 4, "LRU");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 8, "LRU");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 16, "LRU");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << endl;
	outFile << endl;

cout << "Fully Associative Cache (16KB) w/ line size of 32B [LRU]" << endl << " ";
	fullyAssociativeCache(hits, accesses, inFile, 16384, 32, "LRU");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << endl;
	outFile << endl;

cout << "Fully Associative Cache (16KB) w/ line size of 32B [PLRU]" << endl << " ";
	fullyAssociativeCache(hits, accesses, inFile, 16384, 32, "PLRU");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	outFile << endl;

cout << "2-, 4-, 8-, and 16- Way Set Associative Cache (16KB) w/ line size of 32B [noAllocationOnWriteMiss]" << endl << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 2, "noAllocationOnWriteMiss");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 4, "noAllocationOnWriteMiss");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 8, "noAllocationOnWriteMiss");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 16, "noAllocationOnWriteMiss");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << endl;
	outFile << endl;

cout << "2-, 4-, 8-, and 16- Way Set Associative Cache (16KB) w/ line size of 32B [prefetchNextLine]" << endl << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 2, "prefetchNextLine");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 4, "prefetchNextLine");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 8, "prefetchNextLine");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 16, "prefetchNextLine");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << endl;
	outFile << endl;

cout << "2-, 4-, 8-, and 16- Way Set Associative Cache (16KB) w/ line size of 32B [prefetchNextLineOnMiss]" << endl << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 2, "prefetchNextLineOnMiss");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 4, "prefetchNextLineOnMiss");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 8, "prefetchNextLineOnMiss");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << " ";
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 16, "prefetchNextLineOnMiss");
	printToFile(outFile, hits, accesses);
	outputAndReset(hits, accesses);
	cout << endl;
	outFile << endl;

	inFile.close();
	outFile.close();
	return EXIT_SUCCESS;
}