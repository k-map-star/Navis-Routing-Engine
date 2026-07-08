#include "Parser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <iostream>
#include <cctype>
#include <stdexcept>
using namespace std;

static int parseTimeStr(const string& timeStr) {
    if (timeStr.empty() || timeStr == "None") return 0;
    try {
        int hours = stoi(timeStr.substr(0, 2));
        int mins = stoi(timeStr.substr(3, 2));
        return (hours * 60) + mins;
    } catch (const exception&) {
        return 0;
    }
}

static void calculateAbsoluteTimes(int day, const string& arrStr, const string& depStr, 
                                   int& absArrival, int& absDeparture) {
    int arrMins = parseTimeStr(arrStr);
    int depMins = parseTimeStr(depStr);

    absArrival = ((day - 1) * 1440) + arrMins;

    if (depMins < arrMins) {
        absDeparture = (day * 1440) + depMins; 
    } else {
        absDeparture = ((day - 1) * 1440) + depMins;
    }
}

string Parser::trim(const string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

string Parser::toLower(const string& s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return out;
}

vector<string> Parser::splitCSVLine(const string& line) {
    vector<string> fields;
    stringstream ss(line);
    string field;
    while (getline(ss, field, ',')) {
        fields.push_back(trim(field));
    }
    return fields;
}

bool Parser::loadStations(const string& filepath, unordered_map<string, Station>& stations) {
    ifstream file(filepath);
    if (!file.is_open()) return false;

    string line;
    bool isHeader = true;
    while (getline(file, line)) {
        if (line.empty()) continue;
        if (isHeader) { isHeader = false; continue; }

        const auto fields = splitCSVLine(line);
        if (fields.size() < 4) continue;

        Station s;
        s.code = fields[0];
        s.name = fields[1];
        s.lat = stod(fields[2]);
        s.lon = stod(fields[3]);
        s.is_junction = false;
        stations.emplace(s.code, move(s));
    }
    return true;
}

bool Parser::loadSchedules(const string& filepath, unordered_map<string, vector<ScheduleEntry>>& trainSchedules) {
    ifstream file(filepath);
    if (!file.is_open()) return false;

    string line;
    bool isHeader = true;
    while (getline(file, line)) {
        if (line.empty()) continue;
        if (isHeader) { isHeader = false; continue; }

        const auto fields = splitCSVLine(line);
        if (fields.size() < 8) continue;

        ScheduleEntry entry;
        entry.train_no = fields[0];
        entry.train_name = fields[1];
        entry.station_code = fields[2];
        
        int day = stoi(fields[3]);
        calculateAbsoluteTimes(day, fields[4], fields[5], entry.arrival_min, entry.depart_min);
        entry.seq = stoi(fields[6]);
        
        string days;
        for (size_t i = 7; i < fields.size(); ++i) {
            days += fields[i];
            if (i < fields.size() - 1) days += ",";
        }
        entry.days_of_week = days;

        trainSchedules[entry.train_no].push_back(move(entry));
    }
    return true;
}

void Parser::computeJunctions(unordered_map<string, Station>& stations, const unordered_map<string, vector<ScheduleEntry>>& trainSchedules) {
    unordered_map<string, unordered_set<string>> servingTrains;
    for (const auto& pair : trainSchedules) {
        const string& trainNo = pair.first;
        const vector<ScheduleEntry>& entries = pair.second;
        for (const auto& entry : entries) servingTrains[entry.station_code].insert(trainNo);
    }

    for (auto& pair : servingTrains) {
        const string& code = pair.first;
        const unordered_set<string>& trainSet = pair.second;
        if (stations.count(code) && trainSet.size() >= 10) stations[code].is_junction = true;
    }
}

string Parser::resolveStationCode(const string& input, const unordered_map<string, Station>& stations) {
    const string needle = toLower(trim(input));
    cout << "[DEBUG] Looking for: '" << needle << "'" << endl;

    string upperInput = trim(input);
    for (auto& c : upperInput) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    if (stations.count(upperInput)) {
        return upperInput;
    }

    for (const auto& pair : stations) {
        if (toLower(pair.second.name) == needle) {
            cout << "[DEBUG] Exact Match found: " << pair.first << endl;
            return pair.first;
        }
    }

    vector<string> prefixMatches;
    for (const auto& pair : stations) {
        if (toLower(pair.second.name).find(needle) == 0) {
            prefixMatches.push_back(pair.first);
        }
    }
    
    if (!prefixMatches.empty()) {
        sort(prefixMatches.begin(), prefixMatches.end(), [&](const string& a, const string& b) {
            const auto& stA = stations.at(a);
            const auto& stB = stations.at(b);
            if (stA.is_junction != stB.is_junction) {
                return stA.is_junction; 
            }
            return stA.name.length() < stB.name.length(); 
        });
        cout << "[DEBUG] Prefix Match found: " << prefixMatches.front() << endl;
        return prefixMatches.front();
    }

    vector<string> substringMatches;
    for (const auto& pair : stations) {
        if (toLower(pair.second.name).find(needle) != string::npos) {
            substringMatches.push_back(pair.first);
        }
    }

    if (!substringMatches.empty()) {
        sort(substringMatches.begin(), substringMatches.end(), [&](const string& a, const string& b) {
            const auto& stA = stations.at(a);
            const auto& stB = stations.at(b);
            if (stA.is_junction != stB.is_junction) {
                return stA.is_junction;
            }
            return stA.name.length() < stB.name.length();
        });
        cout << "[DEBUG] Substring Match found: " << substringMatches.front() << endl;
        return substringMatches.front();
    }

    return "";
}
