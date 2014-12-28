#ifndef DECODER_H
#define DECODER_H

#include <unordered_map>
#include <string>
#include <vector>
#include <forward_list>
#include <tuple>

using namespace std;

class Decoder 
{
	public:
		Decoder();
		~Decoder();

		int initialize(char* hodoMapping, char* chamberMapping, char* rtMapping, char* hodoMask, char* triggerRoads);
		int decode(char* hitsFile, char* hodoWriteFile, char* triggerWriteFile);

	private:
		const char DELIMITER = '\t';

		//hashtable for hodo and chamber mappings
		unordered_map<unsigned long, vector<string>>* hcMap;  
		unsigned long hcHashFunction(string a, string b, string c);//corresponding hashfunction

		//hashtable for rt mapping
		unordered_map<string, vector<string>>* rMap;  
		string rHashFunction(string a, string b);//corresponding hashfunction 

		//hashtable for hodomask
		unordered_map< string, forward_list< tuple< string, int, int>>>* mMap;  
		string mHashFunction(string a, string b);//corresponding hashfunction cut off H in hodo detector name?
		forward_list< tuple< string, int, int>> compileHodo(forward_list< string> H1s);

		//datastructures and methods for triggerRoads
		vector< tuple< int, vector< tuple< int, vector< tuple< int, vector< tuple<int, string>>*> >*> >*> >* B4;
		vector< tuple< int, vector< tuple< int, vector< tuple< int, vector< tuple<int, string>>*> >*> >*> >* T4;

		//helper function to initialize mapping
		//app 0 -> hodo 1-> chamber 2-> rt 3->hodomask
		int _init(char* mapping, int app);

		//formatting helper function
		string toString(vector<string> input);
};

#endif
