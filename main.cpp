//-----------------------------------
//			File Decoder
//-----------------------------------
#include <cstring> //strcmp, strtok
#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>

//timing
#include <time.h>

using namespace std;

const int NUM_FILES = 2; //for now raw hits and settings file
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
		//cerr << "Missing -h RAW_HITS_FILE" << endl;
		return 1;
	}
	if (settingsFile == NULL)
	{
		//cerr << "Missing -s SETTINGS_FILE" << endl;
		return 1;
	}
	//cout << basename(rawHitsFile) << endl;	
	clock_t t = clock();
	
	//stdio
	//iostream
	ifstream reader(rawHitsFile);
	ofstream outF("hello2.txt");
	if (reader.is_open())
	{
		int lineSize2 = 60;
		char line2 [lineSize2];
		while (reader.getline(line2,lineSize2))
		{
			outF << line2;
			outF << line2;
			/*outF.write("\n",1);
			outF.write(line2,lineSize2);
			outF.write("\n",1);*/
		}
		reader.close();
		outF.close();
	}
	else
	{
		cerr<< "error" << endl;
		return 1;
	}
	t = clock() - t;
	cout << "It took me " << ((float)t)/CLOCKS_PER_SEC << " seconds to run." << endl;
	
	t = clock();
	FILE * fHits;
	FILE * outFile;
	fHits = fopen(rawHitsFile, "r");
	outFile = fopen(settingsFile, "w");
	int lineSize = 60;
	char line [lineSize];
	if (fHits == NULL) perror("Error opening hits file");
	else
	{
		while(fgets(line, lineSize, fHits) != NULL)
		{
			fputs(line, outFile);
			fputs(line, outFile);
		}
		fclose(fHits);
		fclose(outFile);
	}
	t = clock() - t;
	cout << "It took me " << ((float)t)/CLOCKS_PER_SEC << " seconds to run." << endl;

	return 0;
}

int main(int argc, char** argv)
{
	if (argc != NUM_PARAMS) 
	{ 
		//cerr << "Usage: " << argv[0] << " -h RAW_HITS_FILE -s SETTINGS_FILE"  << endl;
		return 1;
	}
	else
	{
		return run(argv);
	}
}
