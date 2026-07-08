import csv

def search_station():
    query = input("Enter station code or name to search: ").strip().lower()
    if not query:
        return
    
    stations = []
    
    try:
        with open('data/stations.csv', 'r', encoding='utf-8') as f:
            reader = csv.reader(f)
            header = next(reader, None)
            for row in reader:
                if len(row) < 2: continue
                station_code = row[0]
                station_name = row[1]
                
                if query in station_code.lower() or query in station_name.lower():
                    stations.append({
                        'code': station_code,
                        'name': station_name,
                        'lat': row[2] if len(row) > 2 else 'N/A',
                        'lon': row[3] if len(row) > 3 else 'N/A'
                    })
                    
    except FileNotFoundError:
        print("Error: data/stations.csv not found.")
        return
        
    if not stations:
        print(f"\n[!] No station found matching '{query}'.")
        return
        
    print(f"\nFound {len(stations)} matching station(s):\n")
    for st in stations:
        print(f"[{st['code']}] {st['name']} (Lat: {st['lat']}, Lon: {st['lon']})")

if __name__ == '__main__':
    search_station()
