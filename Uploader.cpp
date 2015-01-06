#include "Uploader.h"

Uploader::Uploader()
{
	HIC_Map = new unordered_map<unsigned long, vector<string>>;
	SI_Map = new unordered_map<unsigned long, string>;
	RT_Map = new unordered_map<string, vector<string>>;
	HM_Map = new unordered_map< string, forward_list< tuple< string, int, int>>>;  
	M4 = new vector< tuple<int,vector< tuple<int,vector< tuple<int,vector< tuple<int,string>>*>>*>>*>>;
}

Uploader::~Uploader()
{
	delete HIC_Map; 
	HIC_Map = nullptr;
	delete SI_Map; 
	SI_Map = nullptr;
	delete RT_Map;
	RT_Map = nullptr;
	delete HM_Map;
	HM_Map = nullptr;
	deleteTrigger();
}

string Uploader::toString(vector<string> &input) //should be pointer b/c by value is bad
{
	string output = "";// = "\n";
	for (vector<string>::iterator it = input.begin(); it != --input.end(); ++it)
	{
		output += *(it) + DELIMITER; 
	}
	output += input.back() + "\n";
	return output;
}

//update to bitset<12>
unsigned long Uploader::HIC_HF(string &a, string &b, string &c)
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

unsigned long Uploader::SI_HF(string &a, string &b, string &c)
{
	return HIC_HF(a,b,c);
}

string Uploader::RT_HF(string &a, string &b)
{
	return (a + "_" + b); 
}

string Uploader::HM_HF(string a, string b)
{
	return (a.substr(0,3) + "_" + b); //possibly chop off first H
}

//app_name app_number
//===================
//HODO_INFO	    0
//CHAMBER_INFO  1
//RT		    2
//HODOMASK	    3
//TRIGGER_ROADS	4
//TRIGGER_INFO	5
//SCALER_INFO	6

int Uploader::initialize(ResultSet *res, int app)
{
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
		headerFields = {"roadID", "H1", "H2", "H3", "H4"};
	}
	else if (app == 5)
	{
		headerFields = {"detectorName", "elementID","triggerLevel", "rocID", "boardID", "channelID", "tPeak","width"};
	}
	else if (app == 6)
	{
		headerFields = {"rocID", "boardID", "channelID", "scalerName"};
	}
	else
	{
		cerr << "Internal error" << endl;
		return 1;
	}

	vector<int> index;
	for (auto it = headerFields.begin(); it != headerFields.end(); ++it)
	{
		index.push_back(res->findColumn(*it)); //throws mysql exception if fails
		/*
			if (header[i] == -1) {
				cerr << "Missing some mapping field(s) from " << mapping << endl;
				reader.close();
				return 1;
			}
			*/
	}

	//hodomask helpers
	string prevHodo = ""; 
	forward_list< tuple< string, int, int>> block;

	while (res->next())
	{
		if (app == 0 || app == 1)
		{
			string rocID = res->getString(index[0]);
			string boardID = res->getString(index[1]);
			string channelID = res->getString(index[2]);
			unsigned long key = HIC_HF(rocID, boardID, channelID);
			vector<string> value; 
			if (app == 0)
			{
				value = {to_string(app), res->getString(index[3]), res->getString(index[4]), res->getString(index[5]), res->getString(index[6])};
			}
			else if (app == 1) 
			{
				value = {to_string(app), res->getString(index[3]), res->getString(index[4]), res->getString(index[5]), res->getString(index[6]), res->getString(index[7])};
			}
			(*HIC_Map)[key] = value;
		}
		else if (app == 2)
		{
			string k1 = res->getString(index[0]);
			string k2 = res->getString(index[1]);
			string key = RT_HF(k1,k2);
			vector<string> value = {res->getString(index[2]),res->getString(index[3])};
			(*RT_Map)[key] = value;
		}
		else if (app == 3)
		{
			//hodomask
			string curHodo = res->getString(index[0]);
			if (prevHodo == curHodo)
			{
				block.emplace_front(res->getString(index[1]), res->getInt(index[2]), res->getInt(index[3]));
			}
			else
			{
				if (!block.empty())
				{
					(*HM_Map)[prevHodo] = block;
					prevHodo = curHodo;
					block.clear();
				}
				prevHodo = curHodo;
				block.emplace_front(res->getString(index[1]), res->getInt(index[2]), res->getInt(index[3]));
			}
		}
		else if (app == 4)
		{
			//triggerRoads mapping
			string roadID = res->getString(index[0]);
			int H1 = res->getInt(index[1]);
			int H2 = res->getInt(index[2]);
			int H3 = res->getInt(index[3]);
			int H4 = res->getInt(index[4]);

			bool foundH4 = false;
			tuple<int, string> H1Val = make_tuple(H1, roadID); 
			for (auto it = M4->begin(); it != M4->end(); ++it)
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
				M4->push_back(H4H3H2H1Val);
			}
		}
		else if (app == 5)
		{

		}
		else if (app == 6)
		{
			string rocID = res->getString(index[0]);
			string boardID = res->getString(index[1]);
			string channelID = res->getString(index[2]);
			unsigned long key = SI_HF(rocID, boardID, channelID);
			(*SI_Map)[key] = res->getString(index[3]);
		}
	}
	if (app == 3)
	{
		//get last hodo
		(*HM_Map)[prevHodo] = block;
	}
	return 0;
}

int Uploader::decode(const char* rawFile, string server, string schema, int eventID, string type, Statement *stmt)
{
	vector<string> headerFields;
	if (type == "scaler")
	{
		headerFields = {"scalerID","spillID", "spillType", "rocID", "boardID", "channelID", "scalerName", "value", "dataQuality"};
	}
	else if (type == "tdc")
	{
		//headerFields = {"rocID", "boardID", "channelID", "hitID", "spillID", "eventID", "tdcTime"};
		headerFields = {"hitID", "spillID", "eventID", "rocID", "boardID", "channelID", "tdcTime"};
	}
	else
	{
		cerr<<"Error! Type allowed tdc or scaler, type = "<<type<<endl;
		return 1;
	}

	updateTable(stmt, eventID, type, -1);

	int headerSize = headerFields.size();
		
	//open file to read from
	ifstream reader(rawFile);

	//check if file opened correctly
	if (reader.is_open())
	{
		//missing hits file
		//empyt hit file?
		//error checks
		if (~(reader.eof())) 
		{
			if (type == "scaler")
			{
				string temp1 = "scaler-"+server+"-"+schema+".out";
				const char* scalerFileName = temp1.data(); //const char?
				ofstream scalerFile(scalerFileName);
				string wordBuffer[headerSize]; 
				string line;
				while (getline(reader, line))
				{
					string word;
					int wordIndex = 0;
					stringstream ss(line);
					while (getline(ss, word, DELIMITER)) 
					{
						wordBuffer[wordIndex] = word;
						wordIndex++;
					}
					string scalerName = (*SI_Map)[SI_HF(wordBuffer[3], wordBuffer[4], wordBuffer[5])];  
					if (scalerName.empty())
					{
						scalerName = "\\N";
					}
					vector<string> scalerInfo = {wordBuffer[0], wordBuffer[1], wordBuffer[2], wordBuffer[3], wordBuffer[4], wordBuffer[5], scalerName, wordBuffer[7], wordBuffer[8]};
					string outLine = toString(scalerInfo);
					if (reader.eof())
					{
						outLine = outLine.substr(0, outLine.size()-1);
					}
					scalerFile.write(outLine.data(), outLine.size());
				}
				scalerFile.close();
				//string loadQuery = "LOAD DATA LOCAL INFILE '" + string(scalerFileName) + "' INTO TABLE " + schema + ".Scaler";
				//cout<<loadQuery<<endl;
				//stmt->executeQuery(loadQuery);
			}
			else //type == tdc
			{
				string temp1 = "hits-"+server+"-"+schema+".out";
				string temp2 = "triggerHits-"+server+"-"+schema+".out";
				string temp3 = "triggerRoads-"+server+"-"+schema+".out";
				const char* hitsFileName = temp1.data(); //const char?
				const char* triggerHitsFileName = temp2.data();
				const char* triggerRoadsFileName = temp3.data();

				ofstream hitsFile(hitsFileName);
				ofstream triggerHitsFile(triggerHitsFileName);
				ofstream triggerRoadsFile(triggerRoadsFileName);
				
				string wordBuffer[headerSize]; 
				string prevEvent = ""; 
				bool first = true;
				vector<vector<string>> storage;
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
				string line;
				while (getline(reader, line))
				{
					/*
					wordBuffer
					===========
					0 hitID
					1 spillID
					2 eventID
					3 rocID
					4 boardID
					5 channelID
					6 tdcTime

					hitInfo
					==========
					0 hitID
					1 spillID
					2 eventID
					3 detectorName
					4 elementID
					5 tdcTime
					6 inTime
					7 masked
					8 driftTime
					9 driftDistance
					10 resolution
					11 dataQuality
					 
					triggerHitInfo
					==============
					0 hitID
					1 spillID
					2 eventID
					3 detectorName
					4 elementID
					5 triggerLevel
					6 tdcTime
					7 inTime
					8 errorBits
					9 dataQuality 
					
					hashData
					=================
					0 app (-1 no match 0 hodo 1 chamber)

					hodo
					------------
					1 detectorName
					2 elementID
					3 tpeak
					4 width

					chamber:
					-------------
					1 detectorName
					2 elementID
					3 t0
					4 offset
					5 width
					*/
						
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
					vector<string> hashData = (*HIC_Map)[HIC_HF(wordBuffer[3], wordBuffer[4], wordBuffer[5])];  

					if (hashData.empty())
					{
						hashData.push_back("-1");
						for (int i = 0; i < 4; i++)
						{
							hashData.push_back("\\N");
						}
					}
					string inTime; 
					string rocID = wordBuffer[3];
					//if (rocID == 25)
					//{
						//trigger hit 
					//}
					//else
					//{
					/*hitInfo
					==========
					0 hitID
					1 spillID
					2 eventID
					3 detectorName
					4 elementID
					5 tdcTime
					6 inTime
					7 masked
					8 driftTime
					9 driftDistance
					10 resolution
					11 dataQuality*/
					vector<string> hitInfo;
					//universal
					//hitID
					hitInfo.push_back(wordBuffer[0]);
					//spillID
					hitInfo.push_back(wordBuffer[1]);
					//eventID
					hitInfo.push_back(wordBuffer[2]);
					string curEvent = wordBuffer[2];

					int app = atoi(hashData[0].data());
					if (app == -1)
					{
						//no match found

						//detectorName
						hitInfo.push_back("\\N");
						//elementID
						hitInfo.push_back("\\N");
						//tdcTime
						hitInfo.push_back(wordBuffer[6]);
						//inTime
						hitInfo.push_back("0");//lineInfo.push_back("NULL");
						//driftTime
						hitInfo.push_back("\\N");
						//driftDistance
						hitInfo.push_back("\\N");
						//resolution
						hitInfo.push_back("\\N");
					}
					else if (app == 0)
					{
						//hodo match
						if ((hashData[3] != "\\N") && (hashData[4] != "\\N") && (wordBuffer[6] != "\\N"))
						{
							double tpeak = atof(hashData[3].data());
							double width = atof(hashData[4].data());
							double tdcTime = atof(wordBuffer[6].data());
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
						hitInfo.push_back(hashData[1]);
						//elementID
						hitInfo.push_back(hashData[2]);
						//tdcTime
						hitInfo.push_back(wordBuffer[6]);
						//inTime
						hitInfo.push_back(inTime);
						//driftTime
						hitInfo.push_back("\\N");
						//driftDistance
						hitInfo.push_back("\\N");
						//resolution
						hitInfo.push_back("\\N");
					}
					else if (app == 1)
					{
						//chamber match
						if ((hashData[3] != "\\N") && (hashData[4] != "\\N") && (hashData[5] != "\\N") && (wordBuffer[6] != "\\N"))
						{
							double t0 = atof(hashData[3].data());
							double off = atof(hashData[4].data());
							double width = atof(hashData[5].data());
							double tdcTime = atof(wordBuffer[6].data());
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
						if ((wordBuffer[6] != "\\N") && (hashData[3] != "\\N"))
						{
							double tdcTime = atof(wordBuffer[6].data());
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
							driftTime = "\\N";
						}
						
							
						//detectorName
						hitInfo.push_back(hashData[1]);
						//elementID
						hitInfo.push_back(hashData[2]);
						//tdcTime
						hitInfo.push_back(wordBuffer[6]);
						//inTime
						hitInfo.push_back(inTime);
						//driftTime
						hitInfo.push_back(driftTime);
						
						//rt mapping
						if (hashData[1] != "\\N" && driftTime != "\\N")	
						{
							string pCheck = hashData[1];
							if (pCheck.at(0) == 'P') //capital P!
							{
								pCheck = pCheck.substr(0,3);
							}
							vector<string> rtData = (*RT_Map)[RT_HF(pCheck,driftTime)]; //didn't use rHashFunction because already truncated driftTime
							if (rtData.size() == 0) //didn't find anything
							{
								hitInfo.insert(hitInfo.end(), 2, "\\N");
							}
							else
							{
								hitInfo.insert(hitInfo.end(), rtData.begin(), rtData.end());
							}
						}
						else
						{
							hitInfo.insert(hitInfo.end(), 2, "\\N");
						}
					}
					
					//---------------fill line----------------

					if (prevEvent == curEvent)
					{
						if (h1push)
						{
							fillTriggerGroups(hashData, inTimeHodos, BTriggerGroup, TTriggerGroup);
						}
						storage.push_back(hitInfo);
					}
					else
					{
						if (first)
						{
							first = false;
							prevEvent = curEvent;	
							if (h1push)
							{
								fillTriggerGroups(hashData, inTimeHodos, BTriggerGroup, TTriggerGroup);
							}
							storage.push_back(hitInfo);
						}
						else
						{
							analyzeMasking(inTimeHodos, storage, hitsFile, triggerHitsFile);
							analyzeTrigger(prevEvent, BTriggerGroup, TTriggerGroup, triggerRoadsFile);

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
								fillTriggerGroups(hashData, inTimeHodos, BTriggerGroup, TTriggerGroup);
							}
							storage.push_back(hitInfo);
							prevEvent = curEvent;	
						}
					}
				//}
				analyzeMasking(inTimeHodos, storage, hitsFile, triggerHitsFile);
				analyzeTrigger(prevEvent, BTriggerGroup, TTriggerGroup, triggerRoadsFile);
				}
				
				//close files
				hitsFile.close();
				triggerHitsFile.close();
				triggerRoadsFile.close();
			}
		}
		else
		{
			cerr << "Empty raw hits file"<< endl;
			reader.close();
			return 1;
		}
	}
	else
	{
		cerr << "Error opening raw hits file"<< endl;
		return 1;
	}
	reader.close();
	updateTable(stmt, eventID, type, 0);
	return 0;
}

forward_list< tuple< string, int, int>> Uploader::compileInTimeHodos(forward_list<string> &inTimeHodos)
{
	//H1s, forward list with all Hodoscope detectors with inTime = 1 for a given event
	//use HM_Map to decode 
	forward_list< tuple< string, int, int>> maskedDs;
	for (auto it = inTimeHodos.begin(); it != inTimeHodos.end(); ++it)
	{
		//eventually make more complex
		auto tempL = (*HM_Map)[*it];
		for (auto itT = tempL.begin(); itT != tempL.end(); ++itT)
		{
			maskedDs.push_front(*itT); 
		}
	}
	return maskedDs;
}

void Uploader::updateTable(Statement *stmt, int eventID, string type, int num)
{
	stringstream ss;
	ss << "UPDATE decoderInfo SET status=" << num << ", status_history=CONCAT_WS(' ',status_history,'" << num << "') WHERE codaEventID=" << eventID << " AND type='" << type << "'";
	string query = ss.str();
	stmt->execute(query);
}

void Uploader::fillTriggerGroups(vector<string> &hashData, forward_list<string> &inTimeHodos, vector<vector<int>> &BTriggerGroup, vector<vector<int>> &TTriggerGroup)
{
	char thirdLet = hashData[1].data()[2];
	if (thirdLet == 'B')
	{
		inTimeHodos.push_front(HM_HF(hashData[1],hashData[2]));
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
		inTimeHodos.push_front(HM_HF(hashData[1],hashData[2]));
		vector<int>* vec = &TTriggerGroup[hashData[1].data()[1]-'1'];
		int val = atoi(hashData[2].data());
		//TTriggerGroup[hashData[1].data()[1]-'1'].push_back(atoi(hashData[2].data()));
		if (find(vec->begin(), vec->end(), val) == vec->end())
		{
			vec->push_back(val);
		}
	}
}

void Uploader::analyzeMasking(forward_list<string> &inTimeHodos, vector<vector<string>> &storage, ofstream &hitsFile, ofstream &triggerHitsFile)
{
	forward_list<tuple< string, int, int>> bigDs = compileInTimeHodos(inTimeHodos);
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
			(*itL).push_back("\\N");
		}
		string outLine = toString(*itL);
		
		/*if (reader.eof())
		{
			outLine = outLine.substr(0, outLine.size()-1);
		}*/
		hitsFile.write(outLine.data(), outLine.size());
	}
}

void Uploader::analyzeTrigger(string prevEvent, vector<vector<int>> &BTriggerGroup, vector<vector<int>> &TTriggerGroup, ofstream &triggerRoadsFile)
{
	//event = prevEvent; or storage	
	vector< string> eventRoads;
	
	//BTriggers
	auto eventB4s = BTriggerGroup[3];
	for (auto eb4 = eventB4s.begin(); eb4 != eventB4s.end(); ++eb4)
	{
		for (auto mb4 = M4->begin(); mb4 != M4->end(); ++mb4)
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
													string B_is_neg = "-" + get<1>(*mb1);
													eventRoads.push_back(B_is_neg);
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
		for (auto mb4 = M4->begin(); mb4 != M4->end(); ++mb4)
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
		string outLine = prevEvent + DELIMITER + (*road) + "\n";
		//last line problem
		triggerRoadsFile.write(outLine.data(), outLine.size());
	}
}

void Uploader::deleteTrigger()
{
	for (auto m4 = M4->begin(); m4 != M4->end(); ++m4)
	{
		auto M3s = get<1>(*m4);
		for (auto m3 = M3s->begin(); m3 != M3s->end(); ++m3)
		{
			auto M2s = get<1>(*m3);
			for (auto m2 = M2s->begin(); m2 != M2s->end(); ++m2)
			{
				auto M1s = get<1>(*m2);
				delete M1s;
			}
			delete M2s;
		}
		delete M3s;
	}
	delete M4;
}
