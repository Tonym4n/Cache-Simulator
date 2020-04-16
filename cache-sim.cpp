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

//sets the hit rate for a direct mapped cache;
void directMappedCache(int& h, int& a, ifstream& f, int cacheSize, int lineSize)
{
	string flag;
	unsigned long int addr;
	int numOfSets, tag, set, offset;

	numOfSets = cacheSize / lineSize;
	int cache[numOfSets];
	for(int i = 0; i < numOfSets; i++)
		cache[i] = -1;

	while(f >> flag >> std::hex >> addr)
	{
		a++;
		//offset field = rightmost bits based on line size;
		offset = addr & (lineSize - 1);
		//set field = middle bits; shift rightward to remove offset bits;
		set = (addr >> (int)log2(lineSize)) & (numOfSets - 1);
		//tag field = leftmost bits; shift rightward to remove offset + set bits;
		tag = (addr >> ((int)log2(numOfSets) + (int)log2(lineSize))) & (ULONG_MAX);

		//if found, increment hits; else put tag into cache;
		if(cache[set] == tag)
			h++;
		else
			cache[set] = tag;
	}
}

//sets the hit rate for an n-way set associative cache;
//replaces items using either LRU or hot-cold LRU approximation (PLRU) policy;
void setAssociativeCache(int& h, int& a, ifstream& f, int cacheSize, int lineSize, int way, string replacementPolicy)
{
	string flag;
	unsigned long int addr;
	int numOfSets, tag, set, offset;

	assert(way >= 1 && "way cannot be less than 1");
	numOfSets = cacheSize / (lineSize * way);
	int cache[numOfSets][way];
	for(int i = 0; i < numOfSets; i++)
		for(int j = 0; j < way; j++)
			cache[i][j] = -1;
		
	int n = way - 1;
	//use complete binary tree array to model hotColdBits;
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

			//cache[0] = LRU; cache[rightmost end] = MRU;
			//when replacing LRU, shift all elements left and insert at rightmost end;	
			for(int i = 0; i < way; i++)
			{
				//if found;
				if(cache[set][i] == tag)
				{
					h++;
					int lastPos = way - 1;
					for(int j = i; j < way - 1; j++)
					{
						//if found and set still has empty lines;
						//replace last non empty element;
						if(cache[set][j + 1] == -1)
						{
							lastPos = j;
							break;
						}
						cache[set][j] = cache[set][j + 1];
					}
					cache[set][lastPos] = tag;
					break;
				}
				//if not found and set still has empty lines, put tag in next empty line;
				else if(cache[set][i] == -1)
				{
					cache[set][i] = tag;
					break;
				}
				//if not found and set is full, shift everything left and put tag at end;
				else if(i == way - 1)
				{
					for(int j = 0; j < way - 1; j++)
						cache[set][j] = cache[set][j + 1];
					cache[set][i] = tag;
					break;
				}
			}	
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
			cout << endl;
			for(int i = 0; i < way; i++)
				cout << cache[set][i] << " ";
			cout << endl << endl;
//*/


			for(int i = 0; i < way; i++)
			{
				if(cache[set][i] == tag)
				{
					h++;
					break;
				}
				//traverse LRU path;
				//0 means left child is MRU, therefore go to right child;
				//1 means right child is MRU, therefore go to left child;
				else if(i == way - 1)
				{
					//get set index of leaf node;
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

					//j starts at leftmost leaf node;
					//k starts at cache[set][0];
					//when j == x, update its child, which corresponds to a cache line;
					for(int j = (n - 1) / 2, k = 0; j < n; j++, k += 2)
					{
						if(j == x)
						{
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
		}

/*
			for(int i = 0, j = 1, k = 0; i < n; i++, k++)
			{
				if(k == j){cout << endl; j *= 2; k = 0;}
				cout << hotColdArr[set][i] << " ";
			}
			cout << endl;
			for(int i = 0; i < way; i++)
				cout << cache[set][i] << " ";
			cout << endl << endl;
//*/

	}
/***************************************************/
}

//if a store instruction misses, write it directly into memory instead of the cache;
void setAssociativeCacheNoAllocationOnWriteMiss(int& h, int& a, ifstream& f, int cacheSize, int lineSize, int way)
{
	string flag;
	unsigned long int addr;
	int numOfSets, tag, set, offset;

	assert(way >= 1 && "way cannot be less than 1");
	numOfSets = cacheSize / (lineSize * way);
	int cache[numOfSets][way];
	for(int i = 0; i < numOfSets; i++)
		for(int j = 0; j < way; j++)
			cache[i][j] = -1;

	while(f >> flag >> std::hex >> addr)
	{
		a++;
		offset = addr & (lineSize - 1);
		set = (addr >> (int)log2(lineSize)) & (numOfSets - 1);
		tag = (addr >> ((int)log2(numOfSets) + (int)log2(lineSize))) & (ULONG_MAX);

		//LRU replacement policy;
		for(int i = 0; i < way; i++)
		{
			//if found;
			if(cache[set][i] == tag)
			{
				h++;
				int lastPos = way - 1;
				for(int j = i; j < way - 1; j++)
				{
					if(cache[set][j + 1] == -1)
					{
						lastPos = j;
						break;
					}
					cache[set][j] = cache[set][j + 1];
				}
				cache[set][lastPos] = tag;
				break;
			}
			//if not found and set still has empty lines AND instruction isn't store;
			else if(cache[set][i] == -1 && flag != "S")
			{
				cache[set][i] = tag;
				break;
			}
			//if not found and set is full AND instruction isn't store;
			else if(i == way - 1 && flag != "S")
			{
				for(int j = 0; j < way - 1; j++)
					cache[set][j] = cache[set][j + 1];
				cache[set][i] = tag;
				break;
			}
			else if(flag == "S")
			{
				//code to write tag + data directly into memory (store instruction);
			}
		}	
	}
}

//sets the hit rate for a fully associative cache;
void fullyAssociativeCache(int& h, int& a, ifstream& f, int cacheSize, int lineSize, string replacementPolicy)
{
	//a fullyAssociativeCache is simply an n-way 1 set associative cache;
	//cacheSize = numOfSets * lineSize * way;
	int way = cacheSize / lineSize;
	setAssociativeCache(h, a, f, cacheSize, lineSize, way, replacementPolicy);
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
	cout << endl;
	directMappedCache(hits, accesses, inFile, 4096, 32);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	directMappedCache(hits, accesses, inFile, 16384, 32);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	directMappedCache(hits, accesses, inFile, 32768, 32);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;

	setAssociativeCache(hits, accesses, inFile, 16384, 32, 2, "LRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 4, "LRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 8, "LRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 16, "LRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;

	fullyAssociativeCache(hits, accesses, inFile, 16384, 32, "LRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
*/
///*WRONG---------------------------
	fullyAssociativeCache(hits, accesses, inFile, 16384, 32, "PLRU");
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
//  WRONG---------------------------*/
	setAssociativeCacheNoAllocationOnWriteMiss(hits, accesses, inFile, 16384, 32, 2);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	setAssociativeCacheNoAllocationOnWriteMiss(hits, accesses, inFile, 16384, 32, 4);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	setAssociativeCacheNoAllocationOnWriteMiss(hits, accesses, inFile, 16384, 32, 8);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	setAssociativeCacheNoAllocationOnWriteMiss(hits, accesses, inFile, 16384, 32, 16);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;

	inFile.close();
	outFile.close();
	return EXIT_SUCCESS;
}
