
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <chrono>
#include "Parser.hpp"
#include "Graph.hpp"
using namespace std;

namespace {

string formatMinutes(int minutes) {
    const int h = (minutes / 60) % 24;
    const int m = minutes % 60;
    ostringstream oss;
    oss << setw(2) << setfill('0') << h << ":"
        << setw(2) << setfill('0') << m;
    return oss.str();
}

string stationLabel(const unordered_map<string, Station>& stations,
                          const string& code) {
    const auto it = stations.find(code);
    if (it == stations.end()) return code;
    return it->second.name + " (" + code + ")" + (it->second.is_junction ? " [JCT]" : "");
}

bool compareRoutes(const RouteOption& a, const RouteOption& b) {
    if (a.is_direct != b.is_direct) {
        return a.is_direct; 
    }
    const int durationA = a.legs.back().arrival_min - a.legs.front().depart_min;
    const int durationB = b.legs.back().arrival_min - b.legs.front().depart_min;
    if (durationA != durationB) {
        return durationA < durationB; 
    }
    return a.legs.front().depart_min < b.legs.front().depart_min;
}

void printRouteOption(const RouteOption& route, const unordered_map<string, Station>& stations) {
    const int totalDuration = route.legs.back().arrival_min - route.legs.front().depart_min;
    const int durH = totalDuration / 60;
    const int durM = totalDuration % 60;

    if (route.is_direct) {
        const auto& leg = route.legs.front();
        cout << "  [DIRECT] [" << leg.train_no << "] " << leg.train_name << "\n"
                  << "      " << stationLabel(stations, leg.from_code) << " " << formatMinutes(leg.depart_min)
                  << "  ->  " << stationLabel(stations, leg.to_code) << " " << formatMinutes(leg.arrival_min)
                  << "\n      (Total Duration: " << durH << "h " << durM << "m)\n";
    } else {
        const auto& leg1 = route.legs[0];
        const auto& leg2 = route.legs[1];
        cout << "  [1-HOP]  [" << leg1.train_no << "] " << leg1.train_name << "\n"
                  << "      " << stationLabel(stations, leg1.from_code) << " " << formatMinutes(leg1.depart_min)
                  << "  ->  " << stationLabel(stations, leg1.to_code) << " " << formatMinutes(leg1.arrival_min) << "\n"
                  << "      (layover at " << stationLabel(stations, leg1.to_code) << ": " << route.layover_min << " min)\n"
                  << "  [1-HOP]  [" << leg2.train_no << "] " << leg2.train_name << "\n"
                  << "      " << stationLabel(stations, leg2.from_code) << " " << formatMinutes(leg2.depart_min)
                  << "  ->  " << stationLabel(stations, leg2.to_code) << " " << formatMinutes(leg2.arrival_min)
                  << "\n      (Total Duration: " << durH << "h " << durM << "m)\n";
    }
}

int parseDayOfWeekInput(const string& input) {
    string s = input;
    
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    for (auto& c : s) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    
    if (s.empty()) return -1;
    
    if (s == "SUN" || s == "SUNDAY") return 0;
    if (s == "MON" || s == "MONDAY") return 1;
    if (s == "TUE" || s == "TUESDAY") return 2;
    if (s == "WED" || s == "WEDNESDAY") return 3;
    if (s == "THU" || s == "THURSDAY") return 4;
    if (s == "FRI" || s == "FRIDAY") return 5;
    if (s == "SAT" || s == "SATURDAY") return 6;
    
    if (s.size() == 10 && s[4] == '-' && s[7] == '-') {
        try {
            int y = stoi(s.substr(0, 4));
            int m = stoi(s.substr(5, 2));
            int d = stoi(s.substr(8, 2));
            
            if (m >= 1 && m <= 12 && d >= 1 && d <= 31) {
                
                if (m < 3) {
                    m += 12;
                    y -= 1;
                }
                int K = y % 100;
                int J = y / 100;
                int h = (d + 13 * (m + 1) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
                int mapping[] = {6, 0, 1, 2, 3, 4, 5};
                return mapping[h];
            }
        } catch (...) {
            
        }
    }
    
    return -2; 
}

} 

int main() {
    unordered_map<string, Station> stations;
    unordered_map<string, vector<ScheduleEntry>> trainSchedules;

    const vector<string> candidateRoots = { "", "../", "../../" };
    bool loaded = false;
    for (const auto& root : candidateRoots) {
        unordered_map<string, Station> tmpStations;
        unordered_map<string, vector<ScheduleEntry>> tmpSchedules;
        if (Parser::loadStations(root + "data/stations.csv", tmpStations) &&
            Parser::loadSchedules(root + "data/schedules.csv", tmpSchedules) &&
            !tmpStations.empty() && !tmpSchedules.empty()) {
            stations = move(tmpStations);
            trainSchedules = move(tmpSchedules);
            loaded = true;
            break;
        }
    }

    Parser::computeJunctions(stations, trainSchedules);
    const Graph graph(stations, trainSchedules);

    cout << "=========================================\n"
            "  NAVIS - Time-Dependent Railway Router\n"
            "=========================================\n";
    cout << "Loaded " << stations.size() << " stations, "
              << trainSchedules.size() << " train services.\n";
    cout << "Type 'exit' as the origin at any time to quit.\n\n";

    while (true) {
        cout << "Origin station (name or code): ";
        string originInput;
        if (!getline(cin, originInput)) break;
        if (originInput == "exit" || originInput == "quit") break;

        cout << "Destination station (name or code): ";
        string destInput;
        if (!getline(cin, destInput)) break;

        const string originCode = Parser::resolveStationCode(originInput, stations);
        const string destCode = Parser::resolveStationCode(destInput, stations);

        if (originCode.empty() || destCode.empty()) {
            cout << "\n[!] Could not resolve one or both station names. Try the exact station code.\n\n";
            continue;
        }
        if (originCode == destCode) {
            cout << "\n[!] Origin and destination resolved to the same station.\n\n";
            continue;
        }

        cout << "Travel date (YYYY-MM-DD) or day of week (e.g. MON), or leave empty: ";
        string dayInput;
        if (!getline(cin, dayInput)) break;
        
        int travelDayOfWeek = parseDayOfWeekInput(dayInput);
        if (travelDayOfWeek == -2) {
            cout << "\n[!] Invalid date or day of week format. Use YYYY-MM-DD or MON, TUE, etc.\n\n";
            continue;
        }

        cout << "Via station (name or code, leave empty for any): ";
        string viaInput;
        if (!getline(cin, viaInput)) break;
        
        string viaCode;
        if (!viaInput.empty()) {
            viaCode = Parser::resolveStationCode(viaInput, stations);
            if (viaCode.empty()) {
                cout << "\n[!] Could not resolve via station name.\n\n";
                continue;
            }
        }

        double directDistanceKm = 0.0;
        auto prunedSpace = graph.computePrunedSpace(originCode, destCode, directDistanceKm);

        if (!viaCode.empty()) {
            prunedSpace.clear();
            prunedSpace.insert(viaCode);
        }

        cout << "\n-----------------------------------------\n";
        cout << "Route: " << stationLabel(stations, originCode) << "  ->  "
                  << stationLabel(stations, destCode) << "\n";
        if (!viaCode.empty()) {
            cout << "Via: " << stationLabel(stations, viaCode) << "\n";
        }
        if (travelDayOfWeek >= 0) {
            static const string dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
            cout << "Travel Day: " << dayNames[travelDayOfWeek] << "\n";
        } else {
            cout << "Travel Day: Any\n";
        }
        cout << "Direct distance (haversine): " << fixed << setprecision(1)
                  << directDistanceKm << " km\n";
        if (viaCode.empty()) {
            cout << "Search space after pruning (" << Graph::PRUNING_FACTOR
                      << "x detour cap): " << prunedSpace.size() << " / " << stations.size()
                      << " stations retained\n";
        } else {
            cout << "Search space restricted to via station only.\n";
        }
        cout << "-----------------------------------------\n\n";

        auto start_time = std::chrono::high_resolution_clock::now();
        
        vector<RouteOption> direct = graph.findDirectRoutes(originCode, destCode, travelDayOfWeek);
        vector<RouteOption> oneHop = graph.findOneHopRoutes(originCode, destCode, prunedSpace, travelDayOfWeek);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        cout << "[Performance] Routing query executed in: " << duration.count() << " ms\n";

        vector<RouteOption> allRoutes;
        for (auto& r : direct) allRoutes.push_back(move(r));
        for (auto& r : oneHop) allRoutes.push_back(move(r));

        sort(allRoutes.begin(), allRoutes.end(), compareRoutes);

        if (allRoutes.empty()) {
            cout << "No routes found matching your criteria.\n\n";
            continue;
        }

        size_t index = 0;
        size_t total = allRoutes.size();
        const size_t PAGE_SIZE = 10;
        
        while (index < total) {
            size_t limit = min(index + PAGE_SIZE, total);
            cout << "\nShowing routes " << (index + 1) << " to " << limit << " of " << total << ":\n";
            cout << "=========================================================\n";
            for (size_t i = index; i < limit; ++i) {
                cout << "\nRoute #" << (i + 1) << ":\n";
                printRouteOption(allRoutes[i], stations);
            }
            cout << "=========================================================\n";
            
            index = limit;
            if (index < total) {
                cout << "\nPress Enter to show the next " << min(PAGE_SIZE, total - index) << " routes, or type 'n' to stop: ";
                string response;
                getline(cin, response);
                response.erase(0, response.find_first_not_of(" \t\r\n"));
                response.erase(response.find_last_not_of(" \t\r\n") + 1);
                if (response == "n" || response == "no" || response == "N" || response == "No") {
                    break;
                }
            }
        }
        cout << "\n";
    }

    cout << "Goodbye.\n";
    return 0;
}

