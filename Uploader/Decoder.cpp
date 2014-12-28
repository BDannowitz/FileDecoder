#include "Decoder.h"
#include <iostream>
#include <fstream>
#include <sstream> 
#include <stdlib.h>
#include <cmath> 
#include <math.h> //round
#include <algorithm> //find
#include <list>

Decoder::Decoder()
{
	hcMap = nullptr;
	rMap = nullptr;
	mMap = nullptr;
	B4 = nullptr;
	T4 = nullptr;
}

Decoder::~Decoder()
{
	if (hcMap != nullptr)
	{
		delete hcMap; 
		hcMap = nullptr;
	}
	if (rMap != nullptr)
	{
		delete rMap;
		rMap = nullptr;
	}
	if (mMap != nullptr)
	{
		delete mMap;
		mMap = nullptr;
	}
	if (B4 != nullptr)
	{
		//needs more intense delete, search delete
		delete B4;
		B4 = nullptr;
	}
	if (T4 != nullptr)
	{
		//needs more intense delete, search delete
		delete T4;
		T4 = nullptr;
	}
}

string Decoder::toString(vector<string> input)
{
	string output = "\n";
	for (vector<string>::iterator it = input.begin(); it != --input.end(); ++it)
	{
		output += *(it) + DELIMITER; 
	}
	output += input.back();
	return output;
}

unsigned long Decoder::hcHashFunction(string a, string b, string c)
{
	unsigned short d = atoi(a.data());
	unsigned short e = atoi(b.data());
	unsigned short f = atoi(c.data());
	unsigned long out = d;
	out <<= 16;
	out |= e;
	out <<= 16;
	out |= f;
	return out;
}

string Decoder::rHashFunction(string a, string b)
{
	return (a + "_" + b); 
}

string Decoder::mHashFunction(string a, string b)
{
	return (a.substr(0,3) + "_" + b); //possibly chop off first H
}

//app 0 -> hodo mapping
//app 1 -> chamber mapping
//app 2 -> rt mapping
//app 3 -> hodomask
int Decoder::_init(char* mapping, int app)
{
	ifstream reader(mapping);
	if (reader.is_open())
	{
		string line; //used to get line
		int size; //used to loop lines quickly
		//expected header information
		vector<string> headerFields;
		if (app == 0)
		{
			headerFields = {"rocID", "boardID", "channelID", "detectorName", "elementID", "tPeak", "width"};
		}
		else if (app == 1)
		{
			headerFields = {"rocID", "boardID", "channelID", "detectorName", "elementID", "t0", "offset", "width"};
		}
		else if (app == 2)
		{
			headerFields = {"detectorName", "driftTime", "driftDistance", "resolution"};
		}
		else if (app == 3)
		{
			headerFields = {"hodo", "wireDetectorName", "minwire", "maxwire"};
		}
		else if (app == 4)
		{
			headerFields = {"roadID", "detectorHalf", "H1", "H2", "H3", "H4"};
		}
		else
		{
			cerr << "Internal error" << endl;
			return 1;
		}
		int headerSize = headerFields.size();
		int header[headerSize]; 
		for (int i = 0; i < headerSize; i++)
		{
			header[i] = -1;
		}

		//open hodo mapping file
		//first line header info
		if (getline(reader, line))
		{
			string word;
			int wordIndex = 0;
			stringstream ss(line);
			while (getline(ss, word, DELIMITER))
			{
				vector<string>::iterator it = find(headerFields.begin(), headerFields.end(), word);
				if (it != headerFields.end())
				{
					header[it-headerFields.begin()] = wordIndex;
				}
				wordIndex++;
			}
			size = wordIndex;

			//check if found all by finding if -1 exists in header
			for (int i = 0; i < headerSize; i++)
			{
				if (header[i] == -1) {
					cerr << "Missing some mapping field(s) from " << mapping << endl;
					reader.close();
					return 1;
				}
			}
		}
		else
		{
			cerr << "Empty mapping file " << mapping << endl;
			reader.close();
			return 1;
		}

		//load hcMap with hodo mapping
		if (~(reader.eof())) 
		{
			string wordBuffer[size];

			//hodomask helpers
			string prevHodo = ""; 
			forward_list< tuple< string, int, int>> block;

			//loop thru mapping info
			while (getline(reader, line))
			{
				//two objects used to place words into wordBuffer
				string word;
				int wordIndex = 0;

				//loop thru line
				stringstream ss(line);
				while (getline(ss, word, DELIMITER))
				{
					if (wordIndex >= size) 
					{
						wordIndex = -1; //wont process line because too many entries, doesn't follow header pattern
						break; 		
					}
					wordBuffer[wordIndex] = word;
					wordIndex++;
				}
				if (wordIndex == size)
				{
					//get last hodo
					if (app == 0 || app == 1)
					{
						unsigned long key = hcHashFunction(wordBuffer[header[0]], wordBuffer[header[1]], wordBuffer[header[2]]);
						vector<string> value; 
						if (app == 0)
						{
							value = {to_string(app), wordBuffer[header[3]], wordBuffer[header[4]], wordBuffer[header[5]], wordBuffer[header[6]]};
						}
						else if (app == 1) 
						{
							value = {to_string(app), wordBuffer[header[3]], wordBuffer[header[4]], wordBuffer[header[5]], wordBuffer[header[6]], wordBuffer[header[7]]};
						}
						(*hcMap)[key] = value;
					}
					else if (app == 2)
					{
						string key = rHashFunction(wordBuffer[header[0]], wordBuffer[header[1]]);
						vector<string> value = {wordBuffer[header[2]],wordBuffer[header[3]]};
						(*rMap)[key] = value;
					}
					else if (app == 3)
					{
						//hodomask
						string curHodo = wordBuffer[header[0]];
						if (prevHodo == curHodo)
						{
							block.emplace_front(wordBuffer[header[1]], atoi(wordBuffer[header[2]].data()), atoi(wordBuffer[header[3]].data()));
						}
						else
						{
							if (!block.empty())
							{
								(*mMap)[prevHodo] = block;
								prevHodo = curHodo;
								block.clear();
							}
							prevHodo = curHodo;
							block.emplace_front(wordBuffer[header[1]], atoi(wordBuffer[header[2]].data()), atoi(wordBuffer[header[3]].data()));
						}

					}
					else if (app == 4)
					{
						//triggerRoads mapping
						string roadID = wordBuffer[header[0]];
						string detectorHalf = wordBuffer[header[1]];
						int H1 = atoi(wordBuffer[header[2]].data());
						int H2 = atoi(wordBuffer[header[3]].data());
						int H3 = atoi(wordBuffer[header[4]].data());
						int H4 = atoi(wordBuffer[header[5]].data());

						vector< tuple<int,vector< tuple<int,vector< tuple<int,vector< tuple<int, string>>* >>* >>* >> mapping; //should this be a pointer for optimize?
						if (detectorHalf == "B")
						{
							mapping = (*B4);
						}
						else
						{
							mapping = (*T4);
						}
						bool foundH4 = false;
						tuple<int, string> H1Val = make_tuple(H1, roadID); 
						for (auto it = mapping.begin(); it != mapping.end(); ++it)
						{
							if (get<0>(*it) == H4)
							{
								foundH4 = true;
								bool foundH3 = false;
								auto h3s = get<1>(*it);
								for (auto itt = h3s->begin(); itt != h3s->end(); ++itt)
								{
									if (get<0>(*itt) == H3)
									{
										foundH3 = true;
										bool foundH2 = false;
										auto h2s = get<1>(*itt);
										for (auto ittt = h2s->begin(); ittt != h2s->end(); ++ittt)
										{
											if (get<0>(*ittt) == H2)
											{
												foundH2 = true;
												get<1>(*ittt)->push_back(H1Val);
												break;
											}
										}
										if (!foundH2)
										{
											vector< tuple<int,string>>* VecH1Val = new vector< tuple<int,string>>; //combine
											VecH1Val->push_back(H1Val);
											tuple<int,vector< tuple<int,string>>*> H2H1Val = make_tuple(H2, VecH1Val);
											h2s->push_back(H2H1Val);
										}
										break;
									}
								}
								if (!foundH3)
								{
									vector< tuple<int,string>>* VecH1Val = new vector< tuple<int,string>>; //combine
									VecH1Val->push_back(H1Val);
									tuple<int,vector< tuple<int,string>>*> H2H1Val = make_tuple(H2, VecH1Val);
									vector< tuple<int,vector< tuple<int,string>>*>>* VecH2H1Val = new vector< tuple<int,vector< tuple<int,string>>* >>;
									VecH2H1Val->push_back(H2H1Val);
									tuple< int,vector< tuple<int,vector< tuple<int,string>>*>>*> H3H2H1Val = make_tuple(H3, VecH2H1Val);
									h3s->push_back(H3H2H1Val);
								}
								break;
							}
						}
						if (!foundH4)
						{
							vector< tuple<int,string>>* VecH1Val = new vector< tuple<int,string>>; //combine
							VecH1Val->push_back(H1Val);
							tuple<int,vector< tuple<int,string>>*> H2H1Val = make_tuple(H2, VecH1Val);
							vector< tuple<int,vector< tuple<int,string>>*>>* VecH2H1Val = new vector< tuple<int,vector< tuple<int,string>>* >>;
							VecH2H1Val->push_back(H2H1Val);
							tuple< int,vector< tuple<int,vector< tuple<int,string>>*>>*> H3H2H1Val = make_tuple(H3, VecH2H1Val);
							
							vector< tuple<int,vector< tuple<int,vector< tuple<int,string>>*>>*>>* VecH3H2H1Val = new vector< tuple<int,vector< tuple<int,vector< tuple<int,string>>*>>*>>;
							VecH3H2H1Val->push_back(H3H2H1Val);
							tuple<int,vector< tuple<int,vector< tuple<int,vector< tuple<int,string>>*>>*>>*> H4H3H2H1Val = make_tuple(H4, VecH3H2H1Val);
							mapping.push_back(H4H3H2H1Val);
						}
						if (detectorHalf == "B")
						{
							(*B4) = mapping;
						}
						else
						{
							(*T4) = mapping;
						}
					}
				}
			}
			if (app == 3)
			{
				//get last hodo
				(*mMap)[prevHodo] = block;
			}
		}
		else
		{
			cerr << "No mapping information from " << mapping << endl;
			reader.close();
			return 1;
		}
	}
	else
	{
		cerr << "Error opening mapping file " << mapping << endl;
		return 1;
	}
	if (app == 4)
	{
		/*//print trigger mapping
		cout<< sizeof(*B4) <<endl;
		cout<< sizeof(*T4) <<endl;
		cout<<"B list"<<endl;
		for (auto it = B4->begin(); it != B4->end(); ++it)
		{
			if (get<0>(*it) == 3){
			cout<<"H4: "<<get<0>(*it);
			auto h3s = get<1>(*it);
			for (auto itt = h3s->begin(); itt != h3s->end(); ++itt)
			{
				cout<<"    H3: "<<get<0>(*itt);
				auto h2s = get<1>(*itt);
					for (auto ittt = h2s->begin(); ittt != h2s->end(); ++ittt)
					{
						cout<<"    H2: "<<get<0>(*ittt);
						auto h1s = get<1>(*ittt);
						for (auto itttt = h1s->begin(); itttt != h1s->end(); ++itttt)
						{
							cout<<"    H1: "<<(get<0>(*itttt))<< " VALUE: " << (get<1>(*itttt))<<endl;
						}
					}
			}}
		}
		cout<<"T list"<<endl;
		for (auto it = T4->begin(); it != T4->end(); ++it)
		{
			cout<<"H4: "<<get<0>(*it);
			auto h3s = get<1>(*it);
			for (auto itt = h3s->begin(); itt != h3s->end(); ++itt)
			{
				cout<<"    H3: "<<get<0>(*itt);
				auto h2s = get<1>(*itt);
					for (auto ittt = h2s->begin(); ittt != h2s->end(); ++ittt)
					{
						cout<<"    H2: "<<get<0>(*ittt);
						auto h1s = get<1>(*ittt);
						for (auto itttt = h1s->begin(); itttt != h1s->end(); ++itttt)
						{
							cout<<"    H1: "<<(get<0>(*itttt))<< " VALUE: " << (get<1>(*itttt))<<endl;
						}
					}
			}
		}
		*///end print
	}
	reader.close();
	return 0;
}

int Decoder::initialize(char* hodoMapping, char* chamberMapping, char* rtMapping, char* hodoMask, char* triggerRoads)
{
	hcMap = new unordered_map<unsigned long, vector<string>>;
	rMap = new unordered_map<string, vector<string>>;
	mMap = new unordered_map< string, forward_list< tuple< string, int, int>>>;  
	B4 = new vector< tuple<int,vector< tuple<int,vector< tuple<int,vector< tuple<int,string>>*>>*>>*>>;
	T4 = new vector< tuple<int,vector< tuple<int,vector< tuple<int,vector< tuple<int,string>>*>>*>>*>>;
	if (_init(hodoMapping, 0) || _init(chamberMapping, 1) || _init(rtMapping, 2) || _init(hodoMask, 3) || _init(triggerRoads, 4))
	{
		cerr << "Error initializing" << endl;
		return 1;
	}
	return 0;
}	

int Decoder::decode(char* hitsFile, char* hodoWriteFile, char* triggerWriteFile)
{
	vector<string> headerFields = {"rocID", "boardID", "channelID", "hitID", "spillID", "eventID", "tdcTime"};
	int headerSize = headerFields.size();
	int header[headerSize]; 
	for (int i = 0; i < headerSize; i++)
	{
		header[i] = -1;
	}
		
	//open file to read from
	ifstream reader(hitsFile);

	//check if file opened correctly
	if (reader.is_open())
	{
		//number of entries to size array efficently
		int size;
		string line;
		if (getline(reader, line))
		{
			string word;
			int wordIndex = 0;
			stringstream ss(line);
			while (getline(ss, word, DELIMITER))
			{
				vector<string>::iterator it = find(headerFields.begin(), headerFields.end(), word);
				if (it != headerFields.end())
				{
					header[it-headerFields.begin()] = wordIndex;
				}
				wordIndex++;
			}
			size = wordIndex;
			
			for (int i = 0; i < headerSize; i++)
			{
				if (header[i] == -1) {
					cerr << "Missing some hit field(s)" << endl;
					reader.close();
					return 1;
				}
			}
		}
		else
		{
			cerr << "Empty hit file" << endl;
			reader.close();
			return 1;
		}

		if (~(reader.eof())) 
		{
			//open connection to writeFiles
			ofstream hodoOutFile(hodoWriteFile);
			ofstream triggerOutFile(triggerWriteFile);
			
			string hodoHeaderString = "hitID\tspillID\teventID\trocID\tboardID\tchannelID\tdetectorName\telementID\ttdcTime\tinTime\tdriftTime\tdriftDistance\tresolution\tmasked";
			hodoOutFile.write(hodoHeaderString.data(),hodoHeaderString.size());
			string triggerHeaderString = "eventID\troadID";
			triggerOutFile.write(triggerHeaderString.data(),triggerHeaderString.size());
			
			string wordBuffer[size]; 
			string prevEvent = ""; 
			bool first = true;
			list<vector<string>> storage;
			forward_list<string> inTimeHodos;
			vector<vector<int>> BTriggerGroup; //order: B1 B2 B3 B4
			vector<vector<int>> TTriggerGroup; //order: T1 T2 T3 T4
			for (int i = 0; i < 4; i++)
			{
				vector<int> elemB; //list<int> elem;
				vector<int> elemT; 
				BTriggerGroup.push_back(elemB); //combine into one?
				TTriggerGroup.push_back(elemT); 
			}

			//loop thru hits by event block
			while (getline(reader, line))
			{
				// -----lineInfo(OUTPUT)------	
				// 0 hitID
				// 1 spillID
				// 2 eventID
				// 3 rocID
				// 4 boardID
				// 5 channelID
				// 6 detectorName
				// 7 elementID
				// 8 tdcTime
				// 9 inTime
				// 10 driftTime
				// 11 driftDistance
				// 12 resolution
				// 13 masked
				//assign lineInfo.push_back( ... );
				
				// -----HEADER-------
				// 0 rocID
				// 1 boardID
				// 2 channelID
				// 3 hitID
				// 4 spillID
				// 5 eventID
				// 6 tdcTime
				//access thru wordBuffer[header[ ... ]]

				//------hashData-------
				// 0 app (-1 no match 0 hodo 1 chamber)
				// 01 detectorName
				// 02 elementID
				// 03 tpeak
				// 04 width
				// 11 detectorName
				// 12 elementID
				// 13 t0
				// 14 offset
				// 15 width
				vector<string> lineInfo;
				bool h1push = false; //H___ and intime

				//---------------fill line----------------
				string word;
				int wordIndex = 0;
				stringstream ss(line);
				while (getline(ss, word, DELIMITER)) 
				{
					wordBuffer[wordIndex] = word;
					wordIndex++;
				}
				vector<string> hashData = (*hcMap)[hcHashFunction(wordBuffer[header[0]], wordBuffer[header[1]], wordBuffer[header[2]])];  

				if (hashData.empty())
				{
					hashData.push_back("-1");
					for (int i = 0; i < 4; i++)
					{
						hashData.push_back("NULL");
					}
				}
				string inTime; 

				//universal
				//hitID
				lineInfo.push_back(wordBuffer[header[3]]);
				//spillID
				lineInfo.push_back(wordBuffer[header[4]]);
				//eventID
				lineInfo.push_back(wordBuffer[header[5]]);
				string curEvent = wordBuffer[header[5]];
				//rocID
				lineInfo.push_back(wordBuffer[header[0]]);
				//boardID
				lineInfo.push_back(wordBuffer[header[1]]);
				//channelID
				lineInfo.push_back(wordBuffer[header[2]]);

				int app = atoi(hashData[0].data());
				if (app == -1)
				{
					//no match found

					//detectorName
					lineInfo.push_back("NULL");
					//elementID
					lineInfo.push_back("NULL");
					//tdcTime
					lineInfo.push_back(wordBuffer[header[6]]);
					//inTime
					lineInfo.push_back("0");//lineInfo.push_back("NULL");
					//driftTime
					lineInfo.push_back("NULL");
					//driftDistance
					lineInfo.push_back("NULL");
					//resolution
					lineInfo.push_back("NULL");
				}
				else if (app == 0)
				{
					//hodo match
					if ((hashData[3] != "NULL") && (hashData[4] != "NULL") && (wordBuffer[header[6]] != "NULL"))
					{
						double tpeak = atof(hashData[3].data());
						double width = atof(hashData[4].data());
						double tdcTime = atof(wordBuffer[header[6]].data());
						if (abs(tdcTime-tpeak) <= .5 * width) 
						{
							inTime = "1";
							if (hashData[1].data()[0] == 'H') 
							{
								h1push = true;
							}
						}
						else
						{
							inTime = "0";
						}
					}
					else
					{
						inTime = "0";//inTime = "NULL";
					}

					//detectorName
					lineInfo.push_back(hashData[1]);
					//elementID
					lineInfo.push_back(hashData[2]);
					//tdcTime
					lineInfo.push_back(wordBuffer[header[6]]);
					//inTime
					lineInfo.push_back(inTime);
					//driftTime
					lineInfo.push_back("NULL");
					//driftDistance
					lineInfo.push_back("NULL");
					//resolution
					lineInfo.push_back("NULL");
				}
				else if (app == 1)
				{
					//chamber match
					if ((hashData[3] != "NULL") && (hashData[4] != "NULL") && (hashData[5] != "NULL") && (wordBuffer[header[6]] != "NULL"))
					{
						double t0 = atof(hashData[3].data());
						double off = atof(hashData[4].data());
						double width = atof(hashData[5].data());
						double tdcTime = atof(wordBuffer[header[6]].data());
						if ((tdcTime <= (t0 + off)) && (tdcTime >= (t0 + off - width))) 
						{
							inTime = "1";
						}
						else
						{
							inTime = "0";
						}
					}
					else
					{
						inTime = "0";//inTime = "NULL";
					}
					string driftTime;
					if ((wordBuffer[header[6]] != "NULL") && (hashData[3] != "NULL"))
					{
						double tdcTime = atof(wordBuffer[header[6]].data());
						double t0 = atof(hashData[3].data());
						if (tdcTime > t0)
						{
							driftTime = "0.0";
						}
						else
						{
							string temp = to_string(2.5*(round((t0 - tdcTime)/2.5)));
							driftTime = temp.substr(0, temp.find(".") + 2);
						}
					}
					else
					{
						driftTime = "NULL";
					}
					
						
					//detectorName
					lineInfo.push_back(hashData[1]);
					//elementID
					lineInfo.push_back(hashData[2]);
					//tdcTime
					lineInfo.push_back(wordBuffer[header[6]]);
					//inTime
					lineInfo.push_back(inTime);
					//driftTime
					lineInfo.push_back(driftTime);
					
					//rt mapping
					if (hashData[1] != "NULL" && driftTime != "NULL")	
					{
						string pCheck = hashData[1];
						if (pCheck.at(0) == 'P') //capital P!
						{
							pCheck = pCheck.substr(0,3);
						}
						vector<string> rtData = (*rMap)[(pCheck + "_" + driftTime)]; //didn't use rHashFunction because already truncated driftTime
						if (rtData.size() == 0) //didn't find anything
						{
							lineInfo.insert(lineInfo.end(), 2, "NULL");
						}
						else
						{
							lineInfo.insert(lineInfo.end(), rtData.begin(), rtData.end());
						}
					}
					else
					{
						lineInfo.insert(lineInfo.end(), 2, "NULL");
					}
				}
				
				//---------------fill line----------------

				if (prevEvent == curEvent)
				{
					if (h1push)
					{
						char thirdLet = hashData[1].data()[2];
						if (thirdLet == 'B')
						{
							inTimeHodos.push_front(mHashFunction(hashData[1],hashData[2]));
							vector<int>* vec = &BTriggerGroup[hashData[1].data()[1]-'1'];
							int val = atoi(hashData[2].data());
							//BTriggerGroup[hashData[1].data()[1]-'1'].push_back(atoi(hashData[2].data()));
							if (find(vec->begin(), vec->end(), val) == vec->end())
							{
								vec->push_back(val);
							}
						}
						else if (thirdLet == 'T')
						{
							inTimeHodos.push_front(mHashFunction(hashData[1],hashData[2]));
							vector<int>* vec = &TTriggerGroup[hashData[1].data()[1]-'1'];
							int val = atoi(hashData[2].data());
							//TTriggerGroup[hashData[1].data()[1]-'1'].push_back(atoi(hashData[2].data()));
							if (find(vec->begin(), vec->end(), val) == vec->end())
							{
								vec->push_back(val);
							}
						}
					}
					storage.push_back(lineInfo);
				}
				else
				{
					if (first)
					{
						first = false;
						prevEvent = curEvent;	
						if (h1push)
						{
							char thirdLet = hashData[1].data()[2];
							if (thirdLet == 'B')
							{
								inTimeHodos.push_front(mHashFunction(hashData[1],hashData[2]));
								vector<int>* vec = &BTriggerGroup[hashData[1].data()[1]-'1'];
								int val = atoi(hashData[2].data());
								//BTriggerGroup[hashData[1].data()[1]-'1'].push_back(atoi(hashData[2].data()));
								if (find(vec->begin(), vec->end(), val) == vec->end())
								{
									vec->push_back(val);
								}
							}
							else if (thirdLet == 'T')
							{
								inTimeHodos.push_front(mHashFunction(hashData[1],hashData[2]));
								vector<int>* vec = &TTriggerGroup[hashData[1].data()[1]-'1'];
								int val = atoi(hashData[2].data());
								//TTriggerGroup[hashData[1].data()[1]-'1'].push_back(atoi(hashData[2].data()));
								if (find(vec->begin(), vec->end(), val) == vec->end())
								{
									vec->push_back(val);
								}
							}
						}
						storage.push_back(lineInfo);
					}
					else
					{
						//function analize//
						//masking
						forward_list<tuple< string, int, int>> bigDs = compileHodo(inTimeHodos);
						for (auto itL = storage.begin(); itL != storage.end(); ++itL)
						{
							string maskedVal = "0";
							string d = (*itL)[6];
							int elemID = atoi((*itL)[7].data());
							char fLet = d.data()[0];
							if ((fLet == 'D') || (fLet == 'P'))
							{
								for (auto itD = bigDs.begin(); itD != bigDs.end(); ++itD)
								{
									string dName = get<0>(*itD);
									int start = get<1>(*itD);
									int end = get<2>(*itD);
									if ((d == dName) && (elemID >= start) && (elemID <= end)) 
									{
										maskedVal = "1";
										break;
									}
								}
								(*itL).push_back(maskedVal);
							}
							else
							{
								(*itL).push_back("NULL");
							}
							string outLine = toString(*itL);
							hodoOutFile.write(outLine.data(), outLine.size());
						}
						//trigger
						//event = prevEvent; or storage	
						vector< string> eventRoads;
						
						//BTriggers
						auto eventB4s = BTriggerGroup[3];
						for (auto eb4 = eventB4s.begin(); eb4 != eventB4s.end(); ++eb4)
						{
							for (auto mb4 = B4->begin(); mb4 != B4->end(); ++mb4)
							{
								if ((*eb4) == get<0>(*mb4))
								{
									auto eventB3s = BTriggerGroup[2];
									for (auto eb3 = eventB3s.begin(); eb3 != eventB3s.end(); ++eb3)
									{
										auto mapB3s = get<1>(*mb4);
										for (auto mb3 = mapB3s->begin(); mb3 != mapB3s->end(); ++mb3)
										{
											if ((*eb3) == get<0>(*mb3))
											{
												auto eventB2s = BTriggerGroup[1];
												for (auto eb2 = eventB2s.begin(); eb2 != eventB2s.end(); ++eb2)
												{
													auto mapB2s = get<1>(*mb3);
													for (auto mb2 = mapB2s->begin(); mb2 != mapB2s->end(); ++mb2)
													{
														if ((*eb2) == get<0>(*mb2))
														{
															auto eventB1s = BTriggerGroup[0];
															for (auto eb1 = eventB1s.begin(); eb1 != eventB1s.end(); ++eb1)
															{
																auto mapB1s = get<1>(*mb2);
																for (auto mb1 = mapB1s->begin(); mb1 != mapB1s->end(); ++mb1)
																{
																	if ((*eb1) == get<0>(*mb1))
																	{
																		eventRoads.push_back(get<1>(*mb1));
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
						//TTriggers
						eventB4s = TTriggerGroup[3];
						for (auto eb4 = eventB4s.begin(); eb4 != eventB4s.end(); ++eb4)
						{
							for (auto mb4 = T4->begin(); mb4 != T4->end(); ++mb4)
							{
								if ((*eb4) == get<0>(*mb4))
								{
									auto eventB3s = TTriggerGroup[2];
									for (auto eb3 = eventB3s.begin(); eb3 != eventB3s.end(); ++eb3)
									{
										auto mapB3s = get<1>(*mb4);
										for (auto mb3 = mapB3s->begin(); mb3 != mapB3s->end(); ++mb3)
										{
											if ((*eb3) == get<0>(*mb3))
											{
												auto eventB2s = TTriggerGroup[1];
												for (auto eb2 = eventB2s.begin(); eb2 != eventB2s.end(); ++eb2)
												{
													auto mapB2s = get<1>(*mb3);
													for (auto mb2 = mapB2s->begin(); mb2 != mapB2s->end(); ++mb2)
													{
														if ((*eb2) == get<0>(*mb2))
														{
															auto eventB1s = TTriggerGroup[0];
															for (auto eb1 = eventB1s.begin(); eb1 != eventB1s.end(); ++eb1)
															{
																auto mapB1s = get<1>(*mb2);
																for (auto mb1 = mapB1s->begin(); mb1 != mapB1s->end(); ++mb1)
																{
																	if ((*eb1) == get<0>(*mb1))
																	{
																		eventRoads.push_back(get<1>(*mb1));
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
						for (auto road = eventRoads.begin(); road != eventRoads.end(); ++road)
						{
							string outLine = "\n" + prevEvent + DELIMITER + (*road);
							triggerOutFile.write(outLine.data(), outLine.size());
						}
						//function analize//

						//clear triggerGroups
						for (int i = 0; i < 4; i++)	
						{
							BTriggerGroup[i].clear();
							TTriggerGroup[i].clear();
						}

						storage.clear();
						inTimeHodos.clear();
						if (h1push)
						{
							char thirdLet = hashData[1].data()[2];
							if (thirdLet == 'B')
							{
								inTimeHodos.push_front(mHashFunction(hashData[1],hashData[2]));
								vector<int>* vec = &BTriggerGroup[hashData[1].data()[1]-'1'];
								int val = atoi(hashData[2].data());
								//BTriggerGroup[hashData[1].data()[1]-'1'].push_back(atoi(hashData[2].data()));
								if (find(vec->begin(), vec->end(), val) == vec->end())
								{
									vec->push_back(val);
								}
							}
							else if (thirdLet == 'T')
							{
								inTimeHodos.push_front(mHashFunction(hashData[1],hashData[2]));
								vector<int>* vec = &TTriggerGroup[hashData[1].data()[1]-'1'];
								int val = atoi(hashData[2].data());
								//TTriggerGroup[hashData[1].data()[1]-'1'].push_back(atoi(hashData[2].data()));
								if (find(vec->begin(), vec->end(), val) == vec->end())
								{
									vec->push_back(val);
								}
							}
						}
						storage.push_back(lineInfo);
						prevEvent = curEvent;	
					}
				}
			}
			//function analize//
			//masking
			forward_list<tuple< string, int, int>> bigDs = compileHodo(inTimeHodos);
			for (auto itL = storage.begin(); itL != storage.end(); ++itL)
			{
				string maskedVal = "0";
				string d = (*itL)[6];
				int elemID = atoi((*itL)[7].data());
				if ((d.data())[0] == 'D')
				{
					for (auto itD = bigDs.begin(); itD != bigDs.end(); ++itD)
					{
						string dName = get<0>(*itD);
						int start = get<1>(*itD);
						int end = get<2>(*itD);
						if ((d == dName) && (elemID >= start) && (elemID <= end)) 
						{
							maskedVal = "1";
							break;
						}
					}
					(*itL).push_back(maskedVal);
				}
				else
				{
					(*itL).push_back("NULL");
				}
				string outLine = toString(*itL);
				hodoOutFile.write(outLine.data(), outLine.size());
			}
			//trigger
			//event = prevEvent; or storage	
			vector< string> eventRoads;
			
			//BTriggers
			auto eventB4s = BTriggerGroup[3];
			for (auto eb4 = eventB4s.begin(); eb4 != eventB4s.end(); ++eb4)
			{
				for (auto mb4 = B4->begin(); mb4 != B4->end(); ++mb4)
				{
					if ((*eb4) == get<0>(*mb4))
					{
						auto eventB3s = BTriggerGroup[2];
						for (auto eb3 = eventB3s.begin(); eb3 != eventB3s.end(); ++eb3)
						{
							auto mapB3s = get<1>(*mb4);
							for (auto mb3 = mapB3s->begin(); mb3 != mapB3s->end(); ++mb3)
							{
								if ((*eb3) == get<0>(*mb3))
								{
									auto eventB2s = BTriggerGroup[1];
									for (auto eb2 = eventB2s.begin(); eb2 != eventB2s.end(); ++eb2)
									{
										auto mapB2s = get<1>(*mb3);
										for (auto mb2 = mapB2s->begin(); mb2 != mapB2s->end(); ++mb2)
										{
											if ((*eb2) == get<0>(*mb2))
											{
												auto eventB1s = BTriggerGroup[0];
												for (auto eb1 = eventB1s.begin(); eb1 != eventB1s.end(); ++eb1)
												{
													auto mapB1s = get<1>(*mb2);
													for (auto mb1 = mapB1s->begin(); mb1 != mapB1s->end(); ++mb1)
													{
														if ((*eb1) == (get<0>(*mb1)))
														{
															eventRoads.push_back(get<1>(*mb1));
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			//TTriggers
			eventB4s = TTriggerGroup[3];
			for (auto eb4 = eventB4s.begin(); eb4 != eventB4s.end(); ++eb4)
			{
				for (auto mb4 = T4->begin(); mb4 != T4->end(); ++mb4)
				{
					if ((*eb4) == get<0>(*mb4))
					{
						auto eventB3s = TTriggerGroup[2];
						for (auto eb3 = eventB3s.begin(); eb3 != eventB3s.end(); ++eb3)
						{
							auto mapB3s = get<1>(*mb4);
							for (auto mb3 = mapB3s->begin(); mb3 != mapB3s->end(); ++mb3)
							{
								if ((*eb3) == get<0>(*mb3))
								{
									auto eventB2s = TTriggerGroup[1];
									for (auto eb2 = eventB2s.begin(); eb2 != eventB2s.end(); ++eb2)
									{
										auto mapB2s = get<1>(*mb3);
										for (auto mb2 = mapB2s->begin(); mb2 != mapB2s->end(); ++mb2)
										{
											if ((*eb2) == get<0>(*mb2))
											{
												auto eventB1s = TTriggerGroup[0];
												for (auto eb1 = eventB1s.begin(); eb1 != eventB1s.end(); ++eb1)
												{
													auto mapB1s = get<1>(*mb2);
													for (auto mb1 = mapB1s->begin(); mb1 != mapB1s->end(); ++mb1)
													{
														if ((*eb1) == (get<0>(*mb1)))
														{
															eventRoads.push_back(get<1>(*mb1));
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			for (auto road = eventRoads.begin(); road != eventRoads.end(); ++road)
			{
				string outLine = "\n" + prevEvent + DELIMITER + (*road);
				triggerOutFile.write(outLine.data(), outLine.size());
			}
			//function analize//
			hodoOutFile.close();
			triggerOutFile.close();
		}
		else
		{
			cerr << "No hit information" << endl;
			reader.close();
			return 1;
		}
	}
	else
	{
		cerr << "Error opening hits file" << endl;
		return 1;
	}
	reader.close();
	return 0;
}

forward_list< tuple< string, int, int>> Decoder::compileHodo(forward_list< string> H1s)
{
	//H1s, forward list with all Hodoscope detectors with inTime = 1 for a given event
	//use mMap to decode 
	forward_list< tuple< string, int, int>> maskedDs;
	for (auto it = H1s.begin(); it != H1s.end(); ++it)
	{
		//eventually make more complex
		auto tempL = (*mMap)[*it];
		for (auto itT = tempL.begin(); itT != tempL.end(); ++itT)
		{
			maskedDs.push_front(*itT); 
		}
	}
	return maskedDs;
}
