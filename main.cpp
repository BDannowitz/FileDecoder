//-----------------------------------
//			File Decoder
//-----------------------------------
#include <cstring> //strcmp, strtok
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm> //find

//mysql includes
//#include "mysql_driver.h"
#include <cppconn/driver.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>

//timing
#include <time.h>

using namespace std;

const int NUM_FILES = 2; //raw_hits, settings
const int NUM_PARAMS = 1 + 2 * NUM_FILES;
 
char* find(char** files, const char* flag)
{
	for (int i = 1; i < NUM_PARAMS-1; i++)
	{
		if (strcmp(files[i],flag)==0)
		{
			return files[i+1];
		}
	}
	return NULL;
}

int run(char** filenames)
{
	char* rawHitsFile = find(filenames, "-h");
	char* settingsFile = find(filenames, "-s");
	
	if (rawHitsFile == NULL)
	{
		cerr<<"Missing -h RAW_HITS_FILE"<<endl;;
		return 1;
	}
	if (settingsFile == NULL)
	{
		cerr<<"Missing -s SETTINGS_FILE"<<endl;
		return 1;
	}
	
	//information from filename
	//include error catch?
	char* baseFileName = basename(rawHitsFile);
	int runID = atoi(strtok(baseFileName,"+"));
	int eventID = atoi(strtok(NULL,"+"));
	char * server = strtok(NULL,"+");
	server = "e906-db3.fnal.gov";//TESTING
	int port = 3306;
	if (strcmp(server,"seaquel.physics.illinois.edu")==0)
	{
		port = 3283;
	}
	char * schema = strtok(NULL,"+");
	schema = "user_mariusz_dev"; //TESTING
	char * type = strtok(NULL,".");	

	//information from settings
	vector<string> settingName = {"user", "password", "HODOMASK", "RT", "CHAMBERINFO", "HODOINFO", "TRIGGERROADS", "TRIGGERINFO", "SCALERINFO"};
	vector<string> settingValue;
	ifstream setReader(settingsFile);
	if (setReader.is_open())
	{
		string line;
		for (auto it = settingName.begin(); it != settingName.end(); ++it)
		{
			if (getline(setReader,line))
			{
				settingValue.push_back(line);
			}
			else
			{
				cerr<<"Missing setting "<<*it<<endl;
				setReader.close();
				return 1;
			}
		}
		setReader.close();
	}
	else
	{
		cerr<<"Error opening settings file."<<endl;
		return 1;
	}
	const char* user = settingValue[find(settingName.begin(), settingName.end(), "user") - settingName.begin()].data();
	const char* password = settingValue[find(settingName.begin(), settingName.end(), "password") - settingName.begin()].data();
	
	//queries	
	const char* hodoMaskQuery = settingValue[find(settingName.begin(), settingName.end(), "HODOMASK") - settingName.begin()].data();
	const char* RTQuery = settingValue[find(settingName.begin(), settingName.end(), "RT") - settingName.begin()].data();
	const char* chamberInfoQuery = settingValue[find(settingName.begin(), settingName.end(), "CHAMBERINFO") - settingName.begin()].data();
	const char* hodoInfoQuery = settingValue[find(settingName.begin(), settingName.end(), "HODOINFO") - settingName.begin()].data();
	const char* triggerRoadsQuery = settingValue[find(settingName.begin(), settingName.end(), "TRIGGERROADS") - settingName.begin()].data();
	const char* triggerInfoQuery = settingValue[find(settingName.begin(), settingName.end(), "TRIGGERINFO") - settingName.begin()].data();
	const char* scalerInfoQuery = settingValue[find(settingName.begin(), settingName.end(), "SCALERINFO") - settingName.begin()].data();

	try
	{
		sql::Driver *driver;
		sql::Connection *con;
	    sql::Statement *stmt;

		sql::ResultSet *hodoMaskRes;
		sql::ResultSet *RTRes;
		sql::ResultSet *chamberInfoRes;
		sql::ResultSet *hodoInfoRes;
		sql::ResultSet *triggerRoadsRes;
		sql::ResultSet *triggerInfoRes;
		sql::ResultSet *scalerInfoRes;

		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect(server, user, password);
		con->setSchema(schema);
		stmt = con->createStatement();


		//HodoMask
		res = stmt->executeQuery(hodoMaskQuery);
		while (res->next()) {
			cout << res->getString(1) << endl;
		}
		cout<<"next query"<<endl;
		res = stmt->executeQuery(RTQuery);
		while (res->next()) {
			cout << res->getString(1) << endl;
		}
		delete res;

		//cleanup connection
		delete stmt;
		delete con;
	}
	catch (sql::SQLException &e) 
	{
	    cout << "# ERR: SQLException in " << __FILE__;
  		cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
  		cout << "# ERR: " << e.what();
  		cout << " (MySQL error code: " << e.getErrorCode();
  		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
		//if (stmt) delete stmt;
		//if (conn) delete con;
		return 1;
	}
	return 0;
}

int main(int argc, char** argv)
{
	if (argc != NUM_PARAMS) 
	{ 
		cerr<<"Usage: "<<argv[0]<<" -h RAW_HITS_FILE -s SETTINGS_FILE"<<endl; 
		return 1;
	}
	else
	{
		return run(argv);
	}
}
/*	
	clock_t t = clock();
	t = clock() - t;
	printf("It took me %f seconds to run.\n",((float)t)/CLOCKS_PER_SEC);
	*/
