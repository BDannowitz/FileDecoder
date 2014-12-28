#ifndef Uploader_H
#define Uploader_H

#include <stdio.h>
#include <unordered_map>
//#include <string>
#include <vector>
#include <forward_list>
#include <tuple>
//#include <iostream>
//#include <fstream>
//#include <sstream> 
#include <stdlib.h> //atoi
#include <cmath> //absolute value could slow write explicitly <= && >=
#include <math.h> //round
#include <algorithm> //find
#include <list> //why not forward list?

//mysql includes
#include <cppconn/driver.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>


using namespace std;

class Uploader 
{
	public:
		Uploader();
		~Uploader();

		//app_name app_number
		//===================
		//HODO_INFO	    1
		//CHAMBER_INFO  2
		//RT		    3
		//HODOMASK	    4
		//TRIGGER_ROADS	5
		//TRIGGER_INFO	6
		//SCALER_INFO	7
		int initialize(sql::ResultSet *ref, int app)

		int decode(char* hitsFile, char* fileName);

	private:
		const char DELIMITER = '\t';
		//instead of truncating to two decimal only print to 2 decimal

		//datastructures and methods for apps
		//====================================

		//HODOINFO and CHAMBERINFO
		unordered_map<unsigned long, vector<char*>>* HIC_Map;  
		unsigned long HIC_HF(char* a, char* b, char* c);

		//RT
		unordered_map<char*, vector<char*>>* RT_Map;  
		char* RT_HF(char* a, char* b); 

		//HODOMASK
		unordered_map< char*, forward_list< tuple< char*, int, int>>>* HM_Map;  
		char* HM_HF(char* a, char* b); //compare after H?
		forward_list< tuple< char*, int, int>> compileInTimeHodos(forward_list< char*> inTimeHodos); //addition optimization by comparing lists further
		//is this comparing everything? list<tuple<char* list< tuple<int, int>>> needed?

		//TRIGGER_ROAD
		vector< tuple< int, vector< tuple< int, vector< tuple< int, vector< tuple<int, char*>>*> >*> >*> >* B4;
		vector< tuple< int, vector< tuple< int, vector< tuple< int, vector< tuple<int, char*>>*> >*> >*> >* T4;

		//char* _toString(vector<char*> input);
};

#endif
