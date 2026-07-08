#include "Graph.hpp"
#include <cmath>
#include <limits>
using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Graph::Graph(const unordered_map<string, Station>& stations,
             const unordered_map<string, vector<ScheduleEntry>>& trainSchedules)
    : stations_(stations), trainSchedules_(trainSchedules) {}

double Graph::haversine(const string& codeA, const string& codeB) const {
    const auto itA = stations_.find(codeA);
    const auto itB = stations_.find(codeB);
    if (itA == stations_.end() || itB == stations_.end()) {
        return numeric_limits<double>::infinity();
    }

    const auto& a = itA->second;
    const auto& b = itB->second;

    const double lat1 = a.lat * M_PI / 180.0;
    const double lat2 = b.lat * M_PI / 180.0;
    const double dLat = (b.lat - a.lat) * M_PI / 180.0;
    const double dLon = (b.lon - a.lon) * M_PI / 180.0;

    const double sinDLat = sin(dLat / 2.0);
    const double sinDLon = sin(dLon / 2.0);

    const double h = sinDLat * sinDLat +
                      cos(lat1) * cos(lat2) * sinDLon * sinDLon;
    const double c = 2.0 * atan2(sqrt(h), sqrt(1.0 - h));

    return EARTH_RADIUS_KM * c;
}

unordered_set<string> Graph::computePrunedSpace(const string& originCode,
                                                           const string& destCode,
                                                           double& directDistanceKmOut) const {
    directDistanceKmOut = haversine(originCode, destCode);
    const double threshold = directDistanceKmOut * PRUNING_FACTOR;

    unordered_set<string> pruned;
    pruned.reserve(stations_.size());

    for (const auto& pair : stations_) {
        const string& code = pair.first;
        const double detour = haversine(originCode, code) + haversine(code, destCode);
        if (detour <= threshold) {
            pruned.insert(code);
        }
    }
    return pruned;
}

const ScheduleEntry* Graph::findEntry(const vector<ScheduleEntry>& schedule,
                                       const string& stationCode) {
    for (const auto& entry : schedule) {
        if (entry.station_code == stationCode) return &entry;
    }
    return nullptr;
}

vector<Leg> Graph::collectLegs(const string& fromCode, const string& toCode, int travelDayOfWeek) const {
    vector<Leg> legs;

    for (const auto& pair : trainSchedules_) {
        const string& trainNo = pair.first;
        const vector<ScheduleEntry>& schedule = pair.second;
        const ScheduleEntry* fromEntry = findEntry(schedule, fromCode);
        if (!fromEntry) continue;
        const ScheduleEntry* toEntry = findEntry(schedule, toCode);
        if (!toEntry) continue;
        if (fromEntry->seq >= toEntry->seq) continue; 

        if (travelDayOfWeek >= 0) {
            int boardingDay = (fromEntry->depart_min / 1440) + 1;
            int startDayOfWeek = (travelDayOfWeek - boardingDay + 1) % 7;
            if (startDayOfWeek < 0) startDayOfWeek += 7;
            
            static const string dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
            const string& startDayName = dayNames[startDayOfWeek];
            
            if (fromEntry->days_of_week.find(startDayName) == string::npos) {
                continue; 
            }
        }

        Leg leg;
        leg.train_no = trainNo;
        leg.train_name = fromEntry->train_name;
        leg.from_code = fromCode;
        leg.to_code = toCode;
        leg.depart_min = fromEntry->depart_min;
        leg.arrival_min = toEntry->arrival_min;
        legs.push_back(move(leg));
    }
    return legs;
}

vector<RouteOption> Graph::findDirectRoutes(const string& originCode,
                                                  const string& destCode,
                                                  int travelDayOfWeek) const {
    vector<RouteOption> options;
    int baseDay = (travelDayOfWeek >= 0) ? travelDayOfWeek : 1;
    int t_start_week = baseDay * 1440;

    for (const auto& leg : collectLegs(originCode, destCode, travelDayOfWeek)) {
        const auto& schedule = trainSchedules_.at(leg.train_no);
        const ScheduleEntry* fromEntry = findEntry(schedule, originCode);
        const ScheduleEntry* toEntry = findEntry(schedule, destCode);
        if (!fromEntry || !toEntry) continue;

        int boardingDay = (fromEntry->depart_min / 1440) + 1;
        int startDayOfWeek = (baseDay - boardingDay + 1) % 7;
        if (startDayOfWeek < 0) startDayOfWeek += 7;

        int t_dep = startDayOfWeek * 1440 + fromEntry->depart_min;
        int t_arr = t_dep + (toEntry->arrival_min - fromEntry->depart_min);

        RouteOption opt;
        opt.is_direct = true;
        opt.layover_min = 0;
        
        Leg adjusted_leg = leg;
        adjusted_leg.depart_min = t_dep - t_start_week;
        adjusted_leg.arrival_min = t_arr - t_start_week;
        opt.legs = { adjusted_leg };
        options.push_back(move(opt));
    }
    return options;
}

vector<RouteOption> Graph::findOneHopRoutes(const string& originCode,
                                                  const string& destCode,
                                                  const unordered_set<string>& prunedSpace,
                                                  int travelDayOfWeek) const {
    vector<RouteOption> options;

    static const string dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

    for (const auto& junctionCode : prunedSpace) {
        if (junctionCode == originCode || junctionCode == destCode) continue;

        const auto stationIt = stations_.find(junctionCode);
        if (stationIt == stations_.end() || !stationIt->second.is_junction) continue;

        const auto legsIn = collectLegs(originCode, junctionCode, -1);
        if (legsIn.empty()) continue;
        const auto legsOut = collectLegs(junctionCode, destCode, -1);
        if (legsOut.empty()) continue;

        for (const auto& leg1 : legsIn) {
            const auto& schedule1 = trainSchedules_.at(leg1.train_no);
            const ScheduleEntry* fromEntry1 = findEntry(schedule1, originCode);
            const ScheduleEntry* toEntry1 = findEntry(schedule1, junctionCode);
            if (!fromEntry1 || !toEntry1) continue;

            int boardingDay1 = (fromEntry1->depart_min / 1440) + 1;
            int baseDay = (travelDayOfWeek >= 0) ? travelDayOfWeek : 1;

            int startDayOfWeek1 = (baseDay - boardingDay1 + 1) % 7;
            if (startDayOfWeek1 < 0) startDayOfWeek1 += 7;

            if (travelDayOfWeek >= 0) {
                if (fromEntry1->days_of_week.find(dayNames[startDayOfWeek1]) == string::npos) {
                    continue;
                }
            }

            int t_start_week = baseDay * 1440;
            int t_dep1 = startDayOfWeek1 * 1440 + fromEntry1->depart_min;
            int t_arr1 = t_dep1 + (toEntry1->arrival_min - fromEntry1->depart_min);

            for (const auto& leg2 : legsOut) {
                if (leg1.train_no == leg2.train_no) continue; 

                const auto& schedule2 = trainSchedules_.at(leg2.train_no);
                const ScheduleEntry* juncEntry2 = findEntry(schedule2, junctionCode);
                const ScheduleEntry* toEntry2 = findEntry(schedule2, destCode);
                if (!juncEntry2 || !toEntry2) continue;

                int seq2 = (juncEntry2->depart_min / 1440) + 1;
                int t_dep2_static = juncEntry2->depart_min % 1440;

                int d_arr1 = t_arr1 / 1440;
                int chosen_k = -1;
                int chosen_layover = -1;
                int chosen_t_dep2 = -1;

                for (int k = 0; k < 14; ++k) {
                    int d = (d_arr1 + k) % 7;
                    int startDayOfWeek2 = (d - seq2 + 1) % 7;
                    if (startDayOfWeek2 < 0) startDayOfWeek2 += 7;

                    if (juncEntry2->days_of_week.find(dayNames[startDayOfWeek2]) != string::npos) {
                        int t_dep2 = (t_arr1 / 1440) * 1440 + k * 1440 + t_dep2_static;
                        int layover = t_dep2 - t_arr1;
                        if (layover >= LAYOVER_BUFFER_MIN) {
                            chosen_k = k;
                            chosen_layover = layover;
                            chosen_t_dep2 = t_dep2;
                            break;
                        }
                    }
                }

                if (chosen_k == -1) continue;

                Leg actual_leg1 = leg1;
                actual_leg1.depart_min = t_dep1 - t_start_week;
                actual_leg1.arrival_min = t_arr1 - t_start_week;

                Leg actual_leg2 = leg2;
                actual_leg2.depart_min = chosen_t_dep2 - t_start_week;
                actual_leg2.arrival_min = chosen_t_dep2 + (toEntry2->arrival_min - juncEntry2->depart_min) - t_start_week;

                RouteOption opt;
                opt.is_direct = false;
                opt.layover_min = chosen_layover;
                opt.legs = { actual_leg1, actual_leg2 };
                options.push_back(move(opt));
            }
        }
    }
    return options;
}

