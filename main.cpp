//-----------------------------------
//			File Decoder
//-----------------------------------
#include "Uploader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring> //strcmp, strtok
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
using namespace sql;

const int NUM_FILES = 2; //raw_hits, settings
const int NUM_PARAMS = 1 + 2 * NUM_FILES;
 
char* find(char** files, string flag)
{
	for (int i = 1; i < NUM_PARAMS-1; i++)
	{
		if (strcmp(files[i],flag.data())==0)
		{
			return files[i+1];
		}
	}
	return NULL;
}

//delete_if_all cleanup
//{
//}


int run(char** filenames)
{
	char* rawHitsFile = find(filenames, "-h");
	char* settingsFile = find(filenames, "-s");
	string temp = rawHitsFile;
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
	string server = strtok(NULL,"+");
	//server = "e906-db3.fnal.gov";//TESTING
	int port = 3306;
	if (server == "seaquel.physics.illinois.edu")
	{
		port = 3283;
	}
	string schema = strtok(NULL,"+");
	schema = "user_mariusz_dev"; //TESTING
	string type = strtok(NULL,".");	
	const char *rhf = temp.data();

	//information from settings
	//FORMAT
	//1 user
	//2 password
	//3 HODOINFO
	//4 CHAMBERINFO
	//5 RT
	//6 HODOMASK
	//7 TRIGGERROADS
	//8 TRIGGERINFO
	//9 SCALERINFO
	vector<string> settingName = {"user", "password", "HODOMASK", "RT", "CHAMBERINFO", "HODOINFO", "TRIGGERROADS", "TRIGGERINFO", "SCALERINFO"};
	vector<string> queryString;
	string user;
	string password;
	ifstream setReader(settingsFile);
	if (setReader.is_open())
	{
		string line;
		for (auto it = settingName.begin(); it != settingName.end(); ++it)
		{
			if (getline(setReader,line))
			{
				if (*it == "user")
				{
					user = line;	
				}
				else if (*it == "password")
				{
					password = line;
				}
				else
				{
					queryString.push_back(line);
				}
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

	//create Uploader object
	Uploader* UP = new Uploader();

	clock_t t = clock();

	try
	{
		Driver *driver;
		Connection *con;
	 	Statement *stmt;
	        ConnectOptionsMap opts;

		opts["hostName"]=server;
		opts["userName"]=user;
		opts["password"]=password;
		opts["schema"]=schema;
		opts["OPT_LOCAL_INFILE"]=1;

		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect(opts);
		//con = driver->connect(server, user, password); //include port
		//con->setSchema(schema);
		stmt = con->createStatement();
		
		if (type == "scaler")
		{
			ResultSet *res = stmt->executeQuery(*(prev(queryString.end())));
			if (UP->initialize(res,6))
			{
				cerr<<"Problem initializing scalerInfo"<<endl;
				//delete res?
				//return 1;	
			}
			if (UP->decode(rhf, server, schema, eventID, type, stmt))
			{
				cerr<<"Decoding hits file error"<<endl;
				delete res;
			}
			delete res;
			/*string command = "mysql --local-infile=1 --user=" + user + " --host=" + server + " --password=" + password + " -e " + "\"LOAD DATA LOCAL INFILE 'scaler-e906-db3.fnal.gov-user_mariusz_dev.out' INTO TABLE user_mariusz_dev.Scaler;\"";
			//string command = "mysql --user=" + user + " --host=" + server + " --password=" + password + " -e " + "\"SELECT * FROM user_mariusz_dev.Scaler;\"";
			cout << command <<endl;
			system(command.data());*/
			stmt->execute("LOAD DATA LOCAL INFILE 'scaler-e906-db3.fnal.gov-user_mariusz_dev.out' INTO TABLE user_mariusz_dev.Scaler");
		}
		else //type == tdc
		{
			vector<ResultSet *> allResultSets; 
			int appNum = 0;
			for (auto it = queryString.begin(); it != prev(queryString.end()); ++it)
			{
				ResultSet *res = stmt->executeQuery(*it);
				cout<<"init" <<appNum<<endl;
				if (UP->initialize(res,appNum))
				{
					cerr<<"Problem initializing app number "<<appNum<<endl;
					//delete everything
					//return 1;	
				}
				//res->next();
				//cout<<res->getString(1)<<endl;
				allResultSets.push_back(res);
				appNum++;
			}
			//allResultSets[5]->next();	
			//allResultSets[1]->next();	
			//allResultSets[2]->next();	
			//cout << allResultSets[5]->getString(1) <<endl;
			//cout << allResultSets[1]->getString(1) <<endl;
			//cout << allResultSets[2]->getString(1) <<endl;
			if (UP->decode(rhf, server, schema, eventID, type, stmt))
			{
				cerr<<"Decoding hits file error"<<endl;
				for (auto it = allResultSets.begin(); it != allResultSets.end(); ++it)
				{
					delete *it;
				}
				//return 1;	
			}

			for (auto it = allResultSets.begin(); it != allResultSets.end(); ++it)
			{
				delete *it;
			}
		}

		delete UP;
		delete stmt;
		delete con;
	}
	catch (SQLException &e) 
	{
	    cout << "# ERR: SQLException in " << __FILE__;
  		cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
  		cout << "# ERR: " << e.what();
  		cout << " (MySQL error code: " << e.getErrorCode();
  		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
		/*
		delete UP;
		if (stmt) delete stmt;
		if (con) delete con;
		*/
		//big delete
		//return 1;
	}
	//system("rm *.out");
	t = clock() - t;
	printf("It took me %f seconds to run.\n",((float)t)/CLOCKS_PER_SEC);
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
