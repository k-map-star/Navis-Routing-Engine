import csv

def search_train():
    query = input("Enter train number or name to search: ").strip().lower()
    if not query:
        return
    
    trains = {}
    
    try:
        with open('data/schedules.csv', 'r', encoding='utf-8') as f:
            reader = csv.reader(f)
            header = next(reader, None)
            for row in reader:
                if len(row) < 8: continue
                train_no = row[0]
                train_name = row[1]
                station = row[2]
                
                if query in train_no.lower() or query in train_name.lower():
                    if train_no not in trains:
                        trains[train_no] = {'name': train_name, 'stops': []}
                    trains[train_no]['stops'].append(station)
                    
    except FileNotFoundError:
        print("Error: data/schedules.csv not found.")
        return
        
    if not trains:
        print(f"\n[!] No train found matching '{query}'.")
        return
        
    print(f"\nFound {len(trains)} matching train(s):\n")
    for tno, tdata in trains.items():
        print(f"[{tno}] {tdata['name']}")
        stops_str = " -> ".join(tdata['stops'])
        if len(stops_str) > 100:
            stops_str = stops_str[:100] + "... (truncated)"
        print(f"  Route: {stops_str}")
        print(f"  Total stops: {len(tdata['stops'])}\n")

if __name__ == '__main__':
    search_train()
