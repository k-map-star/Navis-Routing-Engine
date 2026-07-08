#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "Parser.hpp"
using namespace std;

struct Leg {
    string train_no;
    string train_name;
    string from_code;
    string to_code;
    int depart_min = 0;
    int arrival_min = 0;
};

struct RouteOption {
    vector<Leg> legs;
    int layover_min = 0; 
    bool is_direct = true;
};

class Graph {
public:
    Graph(const unordered_map<string, Station>& stations,
          const unordered_map<string, vector<ScheduleEntry>>& trainSchedules);

    double haversine(const string& codeA, const string& codeB) const;

    unordered_set<string> computePrunedSpace(const string& originCode,
                                                        const string& destCode,
                                                        double& directDistanceKmOut) const;

    vector<RouteOption> findDirectRoutes(const string& originCode,
                                               const string& destCode,
                                               int travelDayOfWeek = -1) const;

    vector<RouteOption> findOneHopRoutes(const string& originCode,
                                               const string& destCode,
                                               const unordered_set<string>& prunedSpace,
                                               int travelDayOfWeek = -1) const;

    static constexpr double PRUNING_FACTOR = 1.4;

    static constexpr int LAYOVER_BUFFER_MIN = 20;

private:
    const unordered_map<string, Station>& stations_;
    const unordered_map<string, vector<ScheduleEntry>>& trainSchedules_;

    static constexpr double EARTH_RADIUS_KM = 6371.0;

    static const ScheduleEntry* findEntry(const vector<ScheduleEntry>& schedule,
                                           const string& stationCode);

    vector<Leg> collectLegs(const string& fromCode, const string& toCode, int travelDayOfWeek = -1) const;
};

