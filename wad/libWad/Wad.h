#ifndef P3_WAD_H
#define P3_WAD_H

#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct header{
    string magic;
    int descriptTrack;
    int descriptOffset;
    int descriptNum;
};

struct lump{
    int offset;
    int length;
    string name;
    vector<char> data;
    vector<lump*> elements;
    bool directory;
    bool file;
    bool endMet;
};

class Wad{
public:

    string path;
    vector<char> rawData;
    vector<lump*> lumps;
    header head;

    Wad(){
        string path;
        vector<char> rawData;
        vector<lump*> lumps;
        header head;
    }

    static Wad* loadWad(const string &path);
    string getMagic();
    bool isContent(const string &path);
    bool isDirectory(const string &path);
    int getSize(const string &path);
    int getContents(const string &path, char* buffer, int length, int offset = 0);
    int getDirectory(const string &path, vector<string> *directory);

    static int loadElements(Wad* wad, lump* retLump, int tracker);
    bool traversalDirectory(lump* L, string name, bool hold);
    bool traversalContent(lump* L, string name, bool hold);
    int traversalSize(lump* L, string name, int hold, vector<string>* vecNames, int level);
    int traversalGrabContent(lump* L, string name, char* buf, int hold, int length, int offset, vector<string>* vecNames, int level);
    int traversalGrabNames(lump* L, string name, vector<string> *titles, int hold, vector<string>* vecNames, int level);
    string getPathName(string path);
    void getPathName(string path, vector<string>* names);
};


#endif //P3_WAD_H
