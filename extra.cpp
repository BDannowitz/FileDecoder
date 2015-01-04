//-----------------------------------
//			File Decoder
//-----------------------------------
#include <cstring> //strcmp, strtok
#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
//mysql includes
#include <cppconn/driver.h> //needed?
#include <cppconn/resultset.h>
#include <cppconn/statement.h> //needed?
#include <cppconn/exception.h> //might be needed?

//timing
#include <time.h>

using namespace std;

//Testing
void runTest(sql::ResultSet *ref, char* rawHitsFile)
{
	/*//HodoMask
		res = stmt->executeQuery(hodoMaskQuery);
		while (res->next()) {
			cout << res->getString(1) << endl;
		}
		cout<<"next query"<<endl;
		res = stmt->executeQuery(RTQuery);
		while (res->next()) {
			cout << res->getString(1) << endl;
		}
		*/
	clock_t t = clock();
	const char DELIMITER = '\t';
	//stdio
	FILE * fHits;
	FILE * outFile;
	int lineSize = 65;
	char line [lineSize];
	fHits = fopen(rawHitsFile, "r");
	outFile = fopen("stdio.txt", "w");
	//char line [lineSize];
	list<char**> storage;
	if (fHits == NULL) perror("Error opening hits file");
	else
	{
		while(fgets(line, lineSize, fHits) != NULL)
		{
			char** wordBuffer = new char*[7];
			wordBuffer[0] = strtok(line,"\t");
			if (strcmp(wordBuffer[0],"1443.556\n")==0) {cout<<"1";}
			for (int i = 1; i < 7; i++)
			{
				wordBuffer[i] = strtok(NULL,"\t");
				if (memcmp(wordBuffer[i],"1443.556",8)==0) {cout<<"1";}
				//if (strncmp(wordBuffer[i],"1443.556",8)==0) {cout<<"1";}
			}
			storage.push_back(wordBuffer);
			//if (strcmp(wordBuffer[6],"1443.556\n")==0) {cout<<"1";}
			//fputs(line, outFile);
			//fputs(line, outFile);
		}
		fclose(fHits);
		fclose(outFile);
	}
	cout<< (*(storage.begin()))[0]<<endl;
	cout<< (*(storage.begin()))[6]<<endl;
	t = clock() - t;
	cout << "It took me " << ((float)t)/CLOCKS_PER_SEC << " seconds to run." << endl;

	//fstream
	t = clock();
	ifstream reader(rawHitsFile);
	boost::char_separator<char> sep("\t");
	ofstream outF("fstream.txt");
	string lineF;
	if (reader.is_open())
	{
		while (getline(reader, lineF))
		{
			/*
			string wordBuffer[7];
			*char* wordBuffer[7];
			wordBuffer[0] = strtok(lineF.c_str(),"\t");
			for (int i = 1; i < 7; i++)
			{
				wordBuffer[i] = strtok(NULL,"\t");
			}
			if (strcmp(wordBuffer[6],"1443.556\n")==0) {cout<<"hello";}
			stringstream ss(lineF);
			int wordIndex = 0;
			string word;
			while (getline(ss, word, DELIMITER)) 
			{
				wordBuffer[wordIndex] = word;
				wordIndex++;
			}
			if (wordBuffer[6] == "1443.556\n") {cout<<"1";}
			//lineF += "\n";
			//outF.write(lineF.data(),lineF.size());
			//outF.write(lineF.data(),lineF.size());
			//outF.write("\n",1);
			//or one write
			*/
			boost::tokenizer<boost::char_separator<char>> tok(lineF,sep);
			
			/*for (boost::tokenizer<boost::char_separator<char>>::iterator beg = tok.begin(); beg != tok.end(); ++beg)
			{
				if (*beg == "1443.556") {cout<<"1";}
			}*/
		}
		reader.close();
		outF.close();
	}
	else
	{
		cerr<< "error" << endl;
		return;
	}
	t = clock() - t;
	cout << "It took me " << ((float)t)/CLOCKS_PER_SEC << " seconds to run." << endl;
	
}

int main(int argc, char** argv)
{
	char* rawHitsFile = argv[1];
	
	clock_t t = clock();
	//iostream
	ifstream reader(rawHitsFile);
	ofstream outF("iostream.txt");
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
