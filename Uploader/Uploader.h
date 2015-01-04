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
//#include <math.h> //round, both needed?
#include <algorithm> //find
//#include <list> //why not forward list?
#include <bitset> 

//mysql includes
#include <cppconn/driver.h> //needed?
#include <cppconn/resultset.h>
#include <cppconn/statement.h> //needed?
#include <cppconn/exception.h> //might be needed?


using namespace std;
using namespace sql;

class Uploader 
{
	public:
		Uploader();
		~Uploader();

		//app_name app_number
		//===================
		//HODO_INFO	    0
		//CHAMBER_INFO  1
		//RT		    2
		//HODOMASK	    3
		//TRIGGER_ROADS	4
		//TRIGGER_INFO	5
		//SCALER_INFO	6
		int initialize(ResultSet *res, int app)

		int decode(char* rawFile, string server, string schema, int eventID, string type, Statement *stmt);

	private:
		const char DELIMITER = '\t';
		//instead of truncating to two decimal only print to 2 decimal

		void updateTable(Statement *stmt, int eventID, string type, int num); //update decoderInfo

		//datastructures and methods for apps
		//====================================

		//HODOINFO and CHAMBERINFO
		unordered_map<unsigned long, vector<string>>* HIC_Map;  
		unsigned long HIC_HF(string a, string b, string c);

		//RT
		unordered_map<string, vector<string>>* RT_Map;  
		string RT_HF(string a, string b); 

		//HODOMASK
		unordered_map< string, forward_list< tuple< string, int, int>>>* HM_Map;  
		string HM_HF(string a, string b); //compare after H?
		forward_list< tuple< string, int, int>> compileInTimeHodos(forward_list< string> inTimeHodos); //addition optimization by comparing lists further
		//is this comparing everything? list<tuple<string list< tuple<int, int>>> needed?

		//TRIGGER_ROAD
		vector< tuple< int, vector< tuple< int, vector< tuple< int, vector< tuple<int, string>>*> >*> >*> >* B4;
		vector< tuple< int, vector< tuple< int, vector< tuple< int, vector< tuple<int, string>>*> >*> >*> >* T4;

		//string _toString(vector<string> input);
};

#endif
