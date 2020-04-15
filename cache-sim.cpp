#include <iostream>
#include <climits>
#include <cmath>
#include <fstream>
#include <vector>
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
	int numOfSets, tag, index, offset;

	numOfSets = cacheSize / lineSize;
	int cache[numOfSets];
	for(int i = 0; i < numOfSets; i++)
		cache[i] = -1;

	while(f >> flag >> std::hex >> addr)
	{
		a++;
		//offset field = rightmost bits based on line size;
		offset = addr & (lineSize - 1);
		//index field = middle bits; shift rightward to remove offset bits;
		index = (addr >> (int)log2(lineSize)) & (numOfSets - 1);
		//tag field = leftmost bits; shift rightward to remove offset + index bits;
		tag = (addr >> ((int)log2(numOfSets) + (int)log2(lineSize))) & (ULONG_MAX);

		//if found, increment hits; else put tag into cache;
		if(cache[index] == tag)
			h++;
		else
			cache[index] = tag;
	}
}

//sets the hit rate for an n-way set associative cache;
//replaces items using the LRU policy;
void setAssociativeCache(int& h, int& a, ifstream& f, int cacheSize, int lineSize, int way)
{
	string flag;
	unsigned long int addr;
	int numOfSets, tag, index, offset;

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
		index = (addr >> (int)log2(lineSize)) & (numOfSets - 1);
		tag = (addr >> ((int)log2(numOfSets) + (int)log2(lineSize))) & (ULONG_MAX);

		//cache[0] = LRU; cache[rightmost end] = MRU;
		//when replacing LRU, shift all elements left and insert at rightmost end;	
		for(int i = 0; i < way; i++)
		{
			//if found;
			if(cache[index][i] == tag)
			{
				h++;
				int lastPos = way - 1;
				for(int j = i; j < way - 1; j++)
				{
					//if found and set still has empty lines;
					//replace last non empty element;
					if(cache[index][j + 1] == -1)
					{
						lastPos = j;
						break;
					}
					cache[index][j] = cache[index][j + 1];
				}
				cache[index][lastPos] = tag;
				break;
			}
			//if not found and set still has empty lines, put tag in next empty line;
			else if(cache[index][i] == -1)
			{
				cache[index][i] = tag;
				break;
			}
			//if not found and set is full, shift everything left and put tag at end;
			else if(i == way - 1)
			{
				for(int j = 0; j < way - 1; j++)
					cache[index][j] = cache[index][j + 1];
				cache[index][i] = tag;
				break;
			}
		}	
	}
}

//sets the hit rate for a fully associative cache;
void fullyAssociativeCache(int& h, int& a, ifstream& f, int cacheSize, int lineSize)
{
	//a fullyAssociativeCache is simply an n-way 1 set associative cache;
	//cacheSize = numOfSets * lineSize * way;
	int way = cacheSize / lineSize;
	setAssociativeCache(h, a, f, cacheSize, lineSize, way);
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

	setAssociativeCache(hits, accesses, inFile, 16384, 32, 2);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 4);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 8);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
	setAssociativeCache(hits, accesses, inFile, 16384, 32, 16);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;
*/
	fullyAssociativeCache(hits, accesses, inFile, 16384, 32);
	outputAndReset(hits, accesses);
	resetIfstream(inFile);
	cout << endl;

	inFile.close();
	outFile.close();
	return EXIT_SUCCESS;
}
