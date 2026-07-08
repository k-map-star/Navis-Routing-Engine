#pragma once

#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

struct Station {
    string code;   
    string name;   
    double lat = 0.0;
    double lon = 0.0;
    bool is_junction = false; 
};

struct ScheduleEntry {
    string train_no;
    string train_name;
    string station_code;
    int arrival_min = 0; 
    int depart_min = 0;  
    int seq = 0;          
    string days_of_week; 
};

class Parser {
public:
    
    static bool loadStations(const string& filepath,
                              unordered_map<string, Station>& stations);

    static bool loadSchedules(const string& filepath,
                               unordered_map<string, vector<ScheduleEntry>>& trainSchedules);

    static void computeJunctions(unordered_map<string, Station>& stations,
                                  const unordered_map<string, vector<ScheduleEntry>>& trainSchedules);

    static string resolveStationCode(const string& input,
                                           const unordered_map<string, Station>& stations);

private:
    static vector<string> splitCSVLine(const string& line);
    static string toLower(const string& s);
    static string trim(const string& s);
};

