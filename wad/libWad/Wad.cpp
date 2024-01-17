#include "Wad.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
using namespace std;

Wad* Wad::loadWad(const string &path){
//Object allocater; dynamically creates a Wad object and loads the Wad file data from path
//into memory. Caller must deallocate the memory using the delete keyword

    Wad* retWad = new Wad();

    //Read in Raw Data
    ifstream input_file(path, ios::binary | ios::ate);
    ifstream::pos_type pos = input_file.tellg();
    vector<char> rawData(pos);
    input_file.seekg(0, ios::beg);
    input_file.read(&rawData[0], pos);
    retWad->rawData = rawData;


    //Load Header Magic
    int tracker = 0;
    string mag;
    int i = 0;
    for(int i = 0; i < 4; i++){
        mag.push_back(rawData[i]);
    }
    //cout << "Magic: " << mag << endl;
    retWad->head.magic = mag;

    //Load Header Number of Descriptors
    int numD = 0;
    unsigned char arr_numD[4];
    arr_numD[0] = rawData[4];
    arr_numD[1] = rawData[5];
    arr_numD[2] = rawData[6];
    arr_numD[3] = rawData[7];
    memcpy(&numD, arr_numD, sizeof(int));
    //cout << "Number of Descriptors: " << numD << endl;
    retWad->head.descriptNum = numD;
    retWad->head.descriptTrack = numD;

    //Load Header Descriptor Offset
    int dOffset = 0;
    unsigned char arr_dOff[4];
    arr_dOff[0] = rawData[8];
    arr_dOff[1] = rawData[9];
    arr_dOff[2] = rawData[10];
    arr_dOff[3] = rawData[11];
    memcpy(&dOffset, arr_dOff, sizeof(int));
    //cout << "Descriptor Offset: " << dOffset << endl;
    //cout << "-----End of Header-----" << endl;
    tracker = dOffset;
    retWad->head.descriptOffset = dOffset;


    while(retWad->head.descriptTrack != 0){
        lump* retL = new lump();

        //--------------------------------------load in element offset
        int elemOffset = 0;
        unsigned char arr_Off[4];
        arr_Off[0] = rawData[tracker];
        arr_Off[1] = rawData[tracker+1];
        arr_Off[2] = rawData[tracker+2];
        arr_Off[3] = rawData[tracker+3];
        memcpy(&elemOffset, arr_Off, sizeof(int));
        tracker+=4;
        //cout << "Elem offset: " << elemOffset << endl;
        retL->offset = elemOffset;

        //---------------------------------------load in length
        int elemLen = 0;
        unsigned char arr_Len[4];
        arr_Len[0] = rawData[tracker];
        arr_Len[1] = rawData[tracker+1];
        arr_Len[2] = rawData[tracker+2];
        arr_Len[3] = rawData[tracker+3];
        memcpy(&elemLen, arr_Len, sizeof(int));
        tracker+=4;
        //cout << "Elem length: " << elemLen << endl;
        retL->length = elemLen;

        //---------------------------------------------loads in name
        string elemName;
        for(int i = 0; i < 8; i++){
            elemName.push_back(rawData[i+tracker]);
        }
        tracker+=8;
        auto pos = elemName.find('\000');
        if (pos != std::string::npos) {
            elemName.erase(pos);
        }
        //cout << "Name: " << elemName << endl;
        retL->name = elemName;

        //-------------------------------------------Checks if Directory and if so Do Recursion
        //-------------------------------------------If it is a file load in data and add to lumps
        if(elemName.rfind("_END") != string::npos && elemLen == 0){
            retWad->head.descriptTrack-=1;
            continue;
        }
        if(elemLen == 0){
            if(elemName.rfind("_START") != string::npos){

                string del = "_START";
                auto n = retL->name.find(del);
                retL->name.erase(n, del.length());

                retL->directory = true;
                retL->file = false;
                retL->endMet = false;
                tracker = loadElements(retWad,retL,tracker);
            }else{
                retL->directory = true;
                retL->file = false;
                retL->endMet = true;
                for(int i = 0; i < 10; i++){
                    tracker = loadElements(retWad,retL,tracker);
                }
            }
        }else{
            retL->directory = false;
            retL->file = true;
            retL->endMet = true;
            for(int i = elemOffset; i < elemLen+elemOffset; i++){
                retL->data.push_back(rawData[i]);
            }
        }

        retWad->lumps.push_back(retL);
        retWad->head.descriptTrack-=1;
    }

    input_file.close();
    //cout << "Just finished load wad!!!" << endl;
    return retWad;
}

string Wad::getMagic(){
//returns the magic for this WAD data
    return head.magic;
}

bool Wad::traversalDirectory(lump* L, string name, bool hold) {
//helper function for isDirectory
//handles traversal through lumps to find correct one and determine if it is a directory
    if(name.compare("/") == 0){
        hold = true;
    }
    if(L->name == name && L->directory == true){
        hold = true;
    }
    if(hold == true){
        return true;
    }
    if(L->elements.size() != 0){
        for(int i = 0; i < L->elements.size(); i++){
            hold = traversalDirectory(L->elements[i], name, hold);
        }
    }
    if(hold == true){
        return true;
    }
    return false;
}

bool Wad::traversalContent(lump* L, string name, bool hold) {
//helper function for isContent
//handles traversal through lumps to find correct one and determine if it is content
    if(name.compare("/") == 0){
        hold = false;
    }
    if(L->name == name && L->file == true){
        hold = true;
    }
    if(hold == true){
        return true;
    }
    if(L->elements.size() != 0){
        for(int i = 0; i < L->elements.size(); i++){
            hold = traversalContent(L->elements[i], name, hold);
        }
    }
    if(hold == true){
        return true;
    }
    return false;
}

int Wad::traversalSize(lump* L, string name, int hold, vector<string> *vecNames, int level) {
//helper function for getSize
//handles traversal and returns size of correct lump given by vecNames
    if(L->name == name && L->file == true){
        hold = L->length;
    }
    if(hold != 0){
        return hold;
    }
    if(L->elements.size() != 0 && L->name == vecNames->at(level)){
        for(int i = 0; i < L->elements.size(); i++){
            if(hold != 0){
                return hold;
            }
            if(L->elements[i]->name == vecNames->at(level+1)){
                level++;
                hold = traversalSize(L->elements[i], name, hold, vecNames, level);
            }
        }
    }
    if(hold != 0){
        return hold;
    }
    return 0;
}

int Wad::traversalGrabContent(lump *L, string name, char *buf, int hold, int length, int offset, vector<string> *vecNames, int level) {
//helper function for getContents
//handles traversal to find correct lump from vecNames and returns its contents
    if(L->name == name && L->file == true){
        for(int i = 0; i < length; i++){
            buf[i] = L->data[i+offset];
            hold++;
        }
    }
    if(hold != 0){
        return hold;
    }
    if(L->elements.size() != 0 && L->name == vecNames->at(level)){
        for(int i = 0; i < L->elements.size(); i++){
            if(hold != 0){
                return hold;
            }
            if(L->elements[i]->name == vecNames->at(level+1)){
                level++;
                hold = traversalGrabContent(L->elements[i], name, buf, hold, length, offset, vecNames, level);
            }
        }
    }
    if(hold != 0){
        return hold;
    }
    return 0;
}

int Wad::traversalGrabNames(lump *L, string name, vector<string> *titles, int hold, vector<string> *vecNames, int level) {
//helper function for getDirectory
//handles traversal and returns the names of the files of the directory in question
    if(name.compare("/") == 0){
        for(int i = 0; i < this->lumps.size(); i++){
            titles->push_back(this->lumps[i]->name);
            hold++;
        }
    }
    if(L->name == name){
        for(int i = 0; i < L->elements.size(); i++){
            titles->push_back(L->elements[i]->name);
            hold++;
        }
    }
    if(hold != 0){
        return hold;
    }
    if(L->elements.size() != 0 && L->name == vecNames->at(level)){
        for(int i = 0; i < L->elements.size(); i++){
            if(hold != 0){
                return hold;
            }
            if(L->elements[i]->name == vecNames->at(level+1)){
                level++;
                hold = traversalGrabNames(L->elements[i], name, titles, hold, vecNames, level);
            }
        }
    }
    if(hold != 0){
        return hold;
    }
    return 0;
}

bool Wad::isContent(const string &path){
//returns true if path represents content (data), and false otherwise
    string target = "";
    if(path.compare("/") == 0){
        target = "/";
    }else{
        target = getPathName(path);
    }
    bool test = false;
    for(int i = 0; i < this->lumps.size(); i++){
        if(test == true){
            break;
        }
        test = traversalContent(this->lumps[i], target, false);
    }
    return test;
}

bool Wad::isDirectory(const string &path){
//returns true if path represents a directory and false otherwise
    string target = "";
    if(path.compare("/") == 0){
        target = "/";
        return true;
    }else{
        target = getPathName(path);
    }
    bool test = false;
    for(int i = 0; i < this->lumps.size(); i++){
        if(test == true){
            break;
        }
        test = traversalDirectory(this->lumps[i], target, false);
    }
    return test;
}

int Wad::getSize(const string &path){
//if path represents content, returns the number of bytes in its data; otherwise return -1;
    vector<string> vec;
    bool check = this->isContent(path);
    if(check == false){
        return -1;
    }
    string target = "";
    if(path.compare("/") == 0){
        target = "/";
    }else{
        target = getPathName(path);
        getPathName(path, &vec);
    }
    int size = 0;
    for(int i = 0; i < this->lumps.size(); i++){
        if(size != 0){
            break;
        }
        size = traversalSize(this->lumps[i], target, 0, &vec, 0);
    }
    return size;
}

int Wad::getContents(const string &path, char* buffer, int length, int offset){
//If path represents content, copies as many bytes as are available, up to length, of content's
//data into the preexisting buffer. If offset is provided, data should be copied starting from
//that byte in the content. Returns the number of bytes copied into the buffer, or -1 if path
//does not represent content (e.g., if it represents a directory)

//Given:
    //Path
    //Buffer - copy data into it
    //length - amount of data you are copying
    //offset - default 0, where you start copying data in lump

    vector<string> vec;
    bool check = this->isContent(path);
    if(check == false){
        return -1;
    }
    string target = "";
    if(path.compare("/") == 0){
        target = "/";
    }else{
        target = getPathName(path);
        getPathName(path, &vec);
    }
    int ret = 0;
    for(int i = 0; i < this->lumps.size(); i++){
        if(ret != 0){
            break;
        }
        ret = traversalGrabContent(this->lumps[i], target, buffer, 0, length, offset, &vec, 0);
    }
    return ret;
}

int Wad::getDirectory(const string &path, vector<string> *directory){
//If path represents a directory, places entries for immediately contained elements in
//directory. The elements should be placed in the directory in the same order as they
//are found in the WAD file. Returns the number of elements in the directory, or -1
//if path does not represent a directory (e.g., if it represents content)

//travel to the directory named path
//take all content files names and put them in the directory vector
//return vector size at end

    vector<string> vec;
    bool check = this->isDirectory(path);
    if(check == false){
        return -1;
    }
    string target = "";
    if(path.compare("/") == 0){
        target = "/";
    }else{
        target = getPathName(path);
        getPathName(path, &vec);
    }
    int ret = 0;
    for(int i = 0; i < this->lumps.size(); i++){
        if(ret != 0){
            break;
        }
        ret = traversalGrabNames(this->lumps[i], target, directory, 0, &vec, 0);
    }
    return ret;
}

string Wad::getPathName(string path) {
    string lastChar;
    lastChar+=path[path.length()-1];
    string target;
    if(lastChar.compare("/") == 0){
        path.pop_back();
        int pos = path.rfind('/');
        if(pos!=path.npos){
            for(int i = pos+1; i < path.length(); i++){
                target+=path[i];
            }
        }
    }else{
        int pos = path.rfind('/');
        if(pos!=path.npos){
            for(int i = pos+1; i < path.length(); i++){
                target+=path[i];
            }
        }
    }
    return target;
}

void Wad::getPathName(string path, vector<string> *names) {
    string pathHolder = path;
    string holder;
    int pos = 0;
    pathHolder.erase(0,1);
    while(pathHolder.length() != 0){
        if(pathHolder[pos] == '/'){
            names->push_back(holder);
            holder = "";
            pathHolder.erase(0,1);
        }else{
            holder+=pathHolder[pos];
            if(pathHolder.size() == 1){
                names->push_back(holder);
            }
            pathHolder.erase(0,1);
        }
    }
}

int Wad::loadElements(Wad *retWad, lump* retLump, int tracker) {

    lump* Lrec = new lump();

    //-------------------------------------load in element offset
    int elemOffset = 0;
    unsigned char arr_Off[4];
    arr_Off[0] = retWad->rawData[tracker];
    arr_Off[1] = retWad->rawData[tracker+1];
    arr_Off[2] = retWad->rawData[tracker+2];
    arr_Off[3] = retWad->rawData[tracker+3];
    memcpy(&elemOffset, arr_Off, sizeof(int));
    tracker+=4;
    //cout << "Elem offset: " << elemOffset << endl;
    Lrec->offset = elemOffset;

    //--------------------------------------load in element length
    int elemLen = 0;
    unsigned char arr_Len[4];
    arr_Len[0] = retWad->rawData[tracker];
    arr_Len[1] = retWad->rawData[tracker+1];
    arr_Len[2] = retWad->rawData[tracker+2];
    arr_Len[3] = retWad->rawData[tracker+3];
    memcpy(&elemLen, arr_Len, sizeof(int));
    tracker+=4;
    //cout << "Elem length: " << elemLen << endl;
    Lrec->length = elemLen;

    //--------------------------------------load in element name
    string elemName;
    for(int i = 0; i < 8; i++){
        elemName.push_back(retWad->rawData[i+tracker]);
    }
    tracker+=8;
    auto pos = elemName.find('\000');
    if (pos != std::string::npos) {
        elemName.erase(pos);
    }
    //cout << "Name: " << elemName << endl;
    Lrec->name = elemName;

    //--------------------------------------------start of recursion checks
    //---------------------------Directory so do recursion
    if(elemLen == 0){
        Lrec->directory = true;
        Lrec->file = false;
        if(elemName.rfind("_START") != string::npos){

            string del = "_START";
            auto n = Lrec->name.find(del);
            Lrec->name.erase(n, del.length());

            while(elemName.rfind("_END") == string::npos){
                tracker = loadElements(retWad, Lrec, tracker);
                if(Lrec->endMet == true && Lrec->directory == true){
                    retWad->head.descriptTrack-=1;
                    retLump->elements.push_back(Lrec);
                    return tracker;
                }
            }
        }else if(elemName.rfind("_END") != string::npos){
            retWad->head.descriptTrack-=1;
            retLump->endMet = true;
            return tracker;
        }else {
            Lrec->endMet=true;
            for(int i = 0; i < 10; i++){
                tracker = loadElements(retWad,Lrec,tracker);
            }
            retLump->elements.push_back(Lrec);
            retWad->head.descriptTrack-=1;
        }
    }
    if(elemLen != 0){
        //------------------------------------------File so read in data
        Lrec->directory = false;
        Lrec->file = true;
        for(int i = elemOffset; i < elemLen+elemOffset; i++){
            Lrec->data.push_back(retWad->rawData[i]);
        }
        retLump->elements.push_back(Lrec);
        retWad->head.descriptTrack-=1;
    }

    if(retLump->endMet == false){
        tracker = loadElements(retWad,retLump,tracker);
    }
    return tracker;
}
