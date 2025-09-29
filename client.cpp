/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Nilay Alwar	
	UIN: 234001984	
	Date: 9/25/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;


int main (int argc, char *argv[]) {

	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	int m = MAX_MESSAGE;

	bool pActive = false;
	bool tActive = false;
	bool eActive = false;
	bool fActive = false;
	bool cActive = false;
	std::vector<FIFORequestChannel*> channels;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				pActive = true;
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				tActive = true;
				break;
			case 'e':
				eActive = true;
				e = atoi (optarg);
				break;
			case 'f':
				fActive = true;
				filename = optarg;
				break;
			case 'm':
				m = atoi(optarg);
				break;
			case 'c':
				cActive = true;
				break;
		}
	}

	pid_t pid = fork();
	if(pid == -1){
		cerr << "fork failed\n";
		return 1;
	}

	if(pid == 0){
		char *argv[] = { (char*)"server", nullptr};
		execv("./server", argv);
		cerr << "exec failed\n";
		return 1;
	}

    FIFORequestChannel chan1("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&chan1);

	FIFORequestChannel* newChannel = nullptr;

	if(cActive){
		MESSAGE_TYPE newChan = NEWCHANNEL_MSG;
		chan1.cwrite(&newChan, sizeof(MESSAGE_TYPE));
		char chanName[MAX_MESSAGE];
		chan1.cread(&chanName, MAX_MESSAGE);
		FIFORequestChannel* newChannel = new FIFORequestChannel(chanName, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(newChannel);
	}

	FIFORequestChannel chan = *(channels.back());

	if(pActive && !tActive && !eActive){
		std::ofstream file("received/x1.csv");

    	if (!file.is_open()) {
        	std::cerr << "Error: Could not open file for writing.\n";
        	return 1;
    	}
		datamsg x(p, 0, 1);
		char buf[MAX_MESSAGE]; // 256
		for(int i = 0; i < 1000; i++){
    		x.seconds = i*0.004;
			x.ecgno = 1;
	
			memcpy(buf, &x, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg)); // question
			double reply;
			chan.cread(&reply, sizeof(double)); //answer

			x.ecgno = 2;

			memcpy(buf, &x, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg));
			double reply1;
			chan.cread(&reply1, sizeof(double));
			
			file << i*0.004 << "," << reply << "," << reply1 << endl;
		}
	} else if(pActive) {
		char buf[MAX_MESSAGE]; // 256
    	datamsg x(p, t, e);
	
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	
	
    // sending a non-sense message, you need to change this
	if(fActive){
		filemsg fm(0, 0);
		string fname = filename;
	
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);  // I want the file length;

		int64_t filesize = 0;
		chan.cread(&filesize, sizeof(int64_t));

		char* buf3 = new char[m];

		std::ofstream file2("received/" + filename);

		for(int i = 0; i <= filesize / m; ++i){
			filemsg* file_req = (filemsg*)buf2;
			file_req->offset = i*m;
			if(i == filesize / m){
				file_req->length = filesize - i*m;
			} else {
				file_req->length = m;
			}
			chan.cwrite(buf2, len);
			chan.cread(buf3, file_req->length);
			file2.write(buf3, file_req->length);
		}

		delete[] buf2;
		delete[] buf3;
	}

	delete newChannel;
	
	// closing the channel    
    MESSAGE_TYPE mtype = QUIT_MSG;
    chan.cwrite(&mtype, sizeof(MESSAGE_TYPE));
}
