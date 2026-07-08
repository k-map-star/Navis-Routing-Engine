#include <gtest/gtest.h>
#include "Graph.hpp"
#include <unordered_map>
#include <vector>
#include <string>

using namespace std;

class GraphTest : public ::testing::Test {
protected:
    unordered_map<string, Station> stations;
    unordered_map<string, vector<ScheduleEntry>> trainSchedules;
    Graph* graph;

    void SetUp() override {
        // Mock stations
        stations["NDLS"] = {"NDLS", "New Delhi", 28.6139, 77.2090, true};
        stations["HWH"] = {"HWH", "Howrah Jn", 22.5841, 88.3411, true};
        stations["MGS"] = {"MGS", "Deen Dayal Upadhyaya", 25.2818, 83.1226, true};
        
        // Mock schedules
        // Direct train NDLS -> HWH
        trainSchedules["12301"] = {
            {"12301", "Rajdhani Exp", "NDLS", 1, 1000, 1020, 0},
            {"12301", "Rajdhani Exp", "HWH", 2, 2000, 2020, 1500}
        };
        // Train NDLS -> MGS
        trainSchedules["12401"] = {
            {"12401", "Magadh Exp", "NDLS", 1, 600, 620, 0},
            {"12401", "Magadh Exp", "MGS", 1, 1100, 1120, 750}
        };
        // Train MGS -> HWH (wait > 20 mins)
        trainSchedules["12137"] = {
            {"12137", "Punjab Mail", "MGS", 1, 1200, 1220, 750},
            {"12137", "Punjab Mail", "HWH", 2, 1900, 1920, 1500}
        };

        graph = new Graph(stations, trainSchedules);
    }

    void TearDown() override {
        delete graph;
    }
};

TEST_F(GraphTest, HaversineDistanceCorrectness) {
    double distance = graph->haversine("NDLS", "HWH");
    // New Delhi to Howrah is roughly 1295 km
    EXPECT_NEAR(distance, 1303.0, 10.0);
}

TEST_F(GraphTest, FindDirectRoute) {
    auto routes = graph->findDirectRoutes("NDLS", "HWH");
    ASSERT_EQ(routes.size(), 1);
    EXPECT_TRUE(routes[0].is_direct);
    EXPECT_EQ(routes[0].legs[0].train_no, "12301");
}

TEST_F(GraphTest, FindOneHopRoute) {
    double dist;
    auto pruned = graph->computePrunedSpace("NDLS", "HWH", dist);
    auto routes = graph->findOneHopRoutes("NDLS", "HWH", pruned);
    
    // We expect at least one 1-hop route via MGS
    bool found = false;
    for (const auto& r : routes) {
        if (!r.is_direct && r.legs.size() == 2 && 
            r.legs[0].to_code == "MGS" && r.legs[1].from_code == "MGS") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}
