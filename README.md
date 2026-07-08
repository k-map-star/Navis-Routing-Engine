```Navis - Railway Routing Engine```
Navis is a fast, terminalbased routing engine built in C++ that helps you find the best train routes between stations. It uses a static dataset of historical train schedules to simulate routing logic,including calculating direct paths and 1hop connecting journeys 

``` Features```
```Fast Routing :``` Quickly searches for direct trains between your origin and destination.
```Smart Connection :``` Identifies optimal 1hop connecting trains via junction stations with a minimum 20min layover.
```Geographic Pruning :``` Uses the Haversine formula to filter out stations that would cause massive detours,making the search incredibly fast


```Example Output```
```
Origin station (name or code): new delhi
Destination station (name or code): howrah

-----------------------------------------
Route: New Delhi (NDLS) [JCT]  ->  Howrah Jn (HWH)
Direct distance (haversine): 1295.4 km
Search space after pruning (1.4x detour cap): 11 / 12 stations retained
-----------------------------------------

DIRECT TRAINS:
  [12301] Howrah Rajdhani  New Delhi (NDLS) [JCT] 17:00  ->  Howrah Jn (HWH) 09:25

1-HOP CONNECTING TRAINS (via pruned junctions, >=20min layover):
  [12401] Magadh Express  New Delhi (NDLS) [JCT] 10:05  ->  Pandit Deen Dayal Upadhyaya Jn (MGS) [JCT] 17:10
      layover at Pandit Deen Dayal Upadhyaya Jn (MGS) [JCT]: 125 min
  [12137] Punjab Mail  Pandit Deen Dayal Upadhyaya Jn (MGS) [JCT] 19:15  ->  Howrah Jn (HWH) 23:40

```
