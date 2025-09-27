#include<iostream>
#include<vector>
#include<unordered_map>
#include<queue>
#include<stdexcept>
#include<sstream>
#include<thread>
#include<chrono>
#include<ctime>
#include<mutex>
#include<atomic>
#include<cstdlib>
#include<functional>
#include<stack>
#include<fstream>
#include<time.h>
using namespace std;

struct Event{
    int startTime;
    int endTime;
    string description;
    string date;
    bool markedForDeletion;

    Event(string d, int start, int end, string desc){
        date = d;
        startTime = start;
        endTime = end;
        description = desc;
        markedForDeletion = false;
    }
};

struct EventNode{
    int height;
    Event* event;
    EventNode* left;
    EventNode* right;

    EventNode(Event* e){
        event = e;
        height = 1;
        left = right = nullptr;
    }
};

struct DateNode{
    string date;
    EventNode* eventRoot;
    int height;
    DateNode* left;
    DateNode* right;

    DateNode(string d){
        date = d;
        eventRoot = nullptr;
        height = 1;
        left = right = nullptr;
    }
};

struct EventComparator{
    bool operator()(const Event* a, const Event* b){
        if (a->date != b->date) {
            return a->date > b->date;
        }
        return a->startTime > b->startTime;
    }
};

class MinHeap{
	private:
	    priority_queue<Event*, vector<Event*>, EventComparator> pq;
	    unordered_map<string, Event*> eventMap;
	    mutex mtx;
	    
	    string generateKey(const string& date, int startTime) {
        	return date + "_" + to_string(startTime); // Unique key for each event
    	}
	
	public:
	    Event* peek(){
	        lock_guard<mutex> lock(mtx);
	        if (!pq.empty()){
	            return pq.top();
	        }
	        return nullptr;
	    }
	
	   void addEvent(Event* event){
        lock_guard<mutex> lock(mtx);
        string key = generateKey(event->date, event->startTime);
        eventMap[key] = event;
        pq.push(event);
    	}

	    void removeEvent(const string& date, int startTime){
	        lock_guard<mutex> lock(mtx);
	        string key = generateKey(date, startTime);
	        if (eventMap.find(key) != eventMap.end()) {
	            Event* event = eventMap[key];
	            event->markedForDeletion = true; // Mark for lazy removal
	            eventMap.erase(key); // Remove from hash table
	        }
	    }
	
	    Event* getHighestPriorityEvent(){
		    lock_guard<mutex> lock(mtx);
		    while (!pq.empty() && pq.top()->markedForDeletion){
		        pq.pop(); // Lazy removal of marked events
		    }
		    if (!pq.empty()) {
		        return pq.top(); // Return the valid top event
		    }
		    return nullptr;
		}

	
	    void removeHighestPriorityEvent() {
		    lock_guard<mutex> lock(mtx);
		    while (!pq.empty() && pq.top()->markedForDeletion) {
		        pq.pop(); // Remove invalid events
		    }
		    if (!pq.empty()) {
		        Event* topEvent = pq.top();
		        string key = topEvent->date + "_" + to_string(topEvent->startTime);
		        eventMap.erase(key); // Remove from hash table
		        pq.pop(); // Remove from the heap
		    }
		}

	
	    bool isEmpty(){
	        lock_guard<mutex> lock(mtx);
	        return pq.empty();
	    }
};

class AVLTreeScheduler{
	private:
	    DateNode* dateRoot;
	    MinHeap heap;
	    mutex treeMtx;
	    thread notificationThread;
	    atomic<bool> terminateNotification;
	
	    int getHeight(EventNode* node){
	        if (node != nullptr){
	            return node->height;
	        }
	        return 0;
	    }
	
	    int getBalanceFactor(EventNode* node){
	        if (node != nullptr){
	            return getHeight(node->left) - getHeight(node->right);
	        }
	        return 0;
	    }
	
	    EventNode* findEventNode(DateNode* dateNode, int startTime){
	        if (!dateNode) 
				return nullptr;
	        EventNode* current = dateNode->eventRoot;
	        while (current){
	            if (startTime == current->event->startTime){
	                return current;
	            }
	            else if (startTime < current->event->startTime){
	                current = current->left;
	            }
	            else{
	                current = current->right;
	            }
	        }
	        return nullptr;
	    }
	
	    EventNode* rotateRight(EventNode* y){
	        EventNode* x = y->left;
	        EventNode* T2 = x->right;
	        x->right = y;
	        y->left = T2;
	        updateHeight(y);
	        updateHeight(x);
	        return x;
	    }
	
	    EventNode* rotateLeft(EventNode* x){
	        EventNode* y = x->right;
	        EventNode* T2 = y->left;
	        y->left = x;
	        x->right = T2;
	        updateHeight(x);
	        updateHeight(y);
	        return y;
	    }
	
	    void updateHeight(EventNode* node){
	        if (node != nullptr) {
	            node->height = 1 + max(getHeight(node->left), getHeight(node->right));
	        }
	    }
	
	    EventNode* balance(EventNode* node){
	        if (node == nullptr) 
				return nullptr;
	        
	        updateHeight(node);
	        int balanceFactor = getBalanceFactor(node);
	
	        if (balanceFactor > 1){
	            if (getBalanceFactor(node->left) < 0){
	                node->left = rotateLeft(node->left);
	            }
	            return rotateRight(node);
	        }
	        if (balanceFactor < -1){
	            if (getBalanceFactor(node->right) > 0){
	                node->right = rotateRight(node->right);
	            }
	            return rotateLeft(node);
	        }
	        return node;
	    }
	
	    EventNode* insertEvent(EventNode* root, Event* event){
	        if (root == nullptr){
	            return new EventNode(event);
	        }
	        if (event->startTime < root->event->startTime){
	            root->left = insertEvent(root->left, event);
	        }
	        else if (event->startTime > root->event->startTime){
	            root->right = insertEvent(root->right, event);
	        }
	        else{
	            cout << "Error: Event with the same start time already exists.\n";
	            return root;
	        }
	        return balance(root);
	    }
	
	    EventNode* deleteEventByTime(EventNode* root, int startTime){
	        if (root == nullptr){
	            return nullptr;
	        }
	        if (startTime < root->event->startTime){
	            root->left = deleteEventByTime(root->left, startTime);
	        }
	        else if (startTime > root->event->startTime){
	            root->right = deleteEventByTime(root->right, startTime);
	        }
	        else{
	            if (root->left == nullptr){
	                EventNode* temp = root->right;
	                delete root->event;
	                delete root;
	                return temp;
	            }
	            else if (root->right == nullptr){
	                EventNode* temp = root->left;
	                delete root->event;
	                delete root;
	                return temp;
	            }
	
	            EventNode* temp = root->right;
	            while (temp->left){
	                temp = temp->left;
	            }
	            root->event = temp->event;
	            root->right = deleteEventByTime(root->right, temp->event->startTime);
	        }
	        return balance(root);
	    }
	
	    void inOrderEvents(EventNode* root){
	        if (root == nullptr){
	            return;
	        }
	        inOrderEvents(root->left);
	        cout << "[" << root->event->startTime << " - " << root->event->endTime 
	             << "] ---- " << root->event->description << "\n";
	        inOrderEvents(root->right);
	    }
	
	    int getDateHeight(DateNode* node){
	        return (node != nullptr) ? node->height : 0;
	    }
	
	    int getDateBalanceFactor(DateNode* node){
	        return (node != nullptr) ? getDateHeight(node->left) - getDateHeight(node->right) : 0;
	    }
	
	    DateNode* rotateDateRight(DateNode* y){
	        DateNode* x = y->left;
	        DateNode* T2 = x->right;
	        x->right = y;
	        y->left = T2;
	        updateDateHeight(y);
	        updateDateHeight(x);
	        return x;
	    }
	
	    DateNode* rotateDateLeft(DateNode* x){
	        DateNode* y = x->right;
	        DateNode* T2 = y->left;
	        y->left = x;
	        x->right = T2;
	        updateDateHeight(x);
	        updateDateHeight(y);
	        return y;
	    }
	
	    void updateDateHeight(DateNode* node){
	        if (node != nullptr){
	            node->height = 1 + max(getDateHeight(node->left), getDateHeight(node->right));
	        }
	    }
	
	    DateNode* balanceDate(DateNode* node){
	        if (node == nullptr) 
				return nullptr;
	
	        updateDateHeight(node);
	        int balanceFactor = getDateBalanceFactor(node);
	
	        if (balanceFactor > 1){
	            if (getDateBalanceFactor(node->left) < 0){
	                node->left = rotateDateLeft(node->left);
	            }
	            return rotateDateRight(node);
	        }
	        if (balanceFactor < -1){
	            if (getDateBalanceFactor(node->right) > 0){
	                node->right = rotateDateRight(node->right);
	            }
	            return rotateDateLeft(node);
	        }
	        return node;
	    }
	
	    DateNode* insertDate(DateNode* root, string date){
	        if (root == nullptr){
	            return new DateNode(date);
	        }
	        if (date < root->date){
	            root->left = insertDate(root->left, date);
	        }
	        else if (date > root->date){
	            root->right = insertDate(root->right, date);
	        }
	        return balanceDate(root);
	    }
	
	    void deleteTree(DateNode* node){
	        if (node != nullptr){
	            deleteTree(node->left);
	            deleteTree(node->right);
	            deleteEventTree(node->eventRoot);
	            delete node;
	        }
	    }
	
	    void deleteEventTree(EventNode* node){
	        if (node != nullptr){
	            deleteEventTree(node->left);
	            deleteEventTree(node->right);
	            delete node->event;
	            delete node;
	        }
	    }
	
	public:
	    AVLTreeScheduler(){
		    dateRoot = NULL;
		    terminateNotification = false;
		    notificationThread = thread(&AVLTreeScheduler::notificationHandler, this);
		}
	
	
	    ~AVLTreeScheduler(){
	        terminateNotification.store(true);
	        if (notificationThread.joinable()){
	            notificationThread.join();
	        }
	        deleteTree(dateRoot);
	    }
	
	    void notificationHandler(){
	        while (!terminateNotification.load()){
	            Event* nextEvent = heap.peek();
	            if (nextEvent){
	                time_t now = time(nullptr);
	                tm* localTime = localtime(&now);
	                
	                char currentDate[11];
	                strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", localTime);
	                string todayDate(currentDate);
	                
	                if (nextEvent->date == todayDate){
	                    int currentTime = (localTime->tm_hour * 10000) +
	                        (localTime->tm_min * 100) +
	                        localTime->tm_sec;
	
	                    int notifyTime = nextEvent->startTime - 500;
	                    if (currentTime >= notifyTime && currentTime < nextEvent->startTime){
	                        cout << "\n[Notification] Event starting soon................ :\n";
	                        cout << "[" << nextEvent->startTime << " - " << nextEvent->endTime
	                            << "] ---- " << nextEvent->description << "\n";
	                    }
	                    if (currentTime >= nextEvent->startTime){
	                        cout << "\n[Notification] Event Starting Now:\n";
	                        cout << "[" << nextEvent->startTime << " - " << nextEvent->endTime
	                            << "] ---- " << nextEvent->description << "\n";
	                        removeEvent(nextEvent->date, nextEvent->startTime);
	                        heap.removeHighestPriorityEvent();
	                    }
	                }
	            }
	           std::this_thread::sleep_for(std::chrono::seconds(60));
	        }
	    }
	
	    bool hasOverlappingEvents(string date, int startTime, int endTime){
	        lock_guard<mutex> lock(treeMtx);
	        DateNode* dateNode = findDateNode(dateRoot, date);
	        if (!dateNode){
	            return false;
	        }
	
	        function<bool(EventNode*)> checkOverlap = [&](EventNode* node) -> bool {
	            if (!node) return false;
	            if (checkOverlap(node->left)) return true;
	            if ((startTime < node->event->endTime && endTime > node->event->startTime)) {
	                cout << "Overlap detected: [" << node->event->startTime << " - " << node->event->endTime
	                    << "] ---- " << node->event->description << "\n";
	                return true;
	            }
	            return checkOverlap(node->right);
	        };
	
	        return checkOverlap(dateNode->eventRoot);
	    }
		bool isAfterCurrentDateTime(const string& date, int time){
		    time_t rawtime;
		    struct tm* timeinfo;
		    ::time(&rawtime);
		    timeinfo = localtime(&rawtime);
		    
		    char currentDate[11];
		    strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", timeinfo);
		    string todayDate(currentDate);
		    
		    int currentTime = (timeinfo->tm_hour * 10000) + 
		                     (timeinfo->tm_min * 100) + 
		                     timeinfo->tm_sec;
		
		    if (date < todayDate){
		        cout << "Error: Cannot add events for past dates.\n";
		        return false;
		    }
		    
		    if (date == todayDate && time <= currentTime){
		        cout << "Error: Cannot add events for past times on current date.\n";
		        return false;
		    }
		    
		    return true;
		}
	
	    void addEvent(string date, int startTime, int endTime, string description){
		    if (!isAfterCurrentDateTime(date, startTime)){
		        return;
		    }
		    
		    if (hasOverlappingEvents(date, startTime, endTime)){
		            cout << "Cannot add event, it overlaps with an existing event.\n";
		            return;
		        }
		
		        lock_guard<mutex> lock(treeMtx);
		        DateNode* dateNode = findDateNode(dateRoot, date);
		        if (dateNode == nullptr){
		            dateRoot = insertDate(dateRoot, date);
		            dateNode = findDateNode(dateRoot, date);
		        }
		        Event* newEvent = new Event(date, startTime, endTime, description);
		        dateNode->eventRoot = insertEvent(dateNode->eventRoot, newEvent);
		        heap.addEvent(newEvent);
		    cout << "Event added successfully \n";
	    }
	

void updateEvent(string date, int startTime, int newStartTime, int newEndTime, const string& newDescription) {
    // First validate the new time is in the future
    if (!isAfterCurrentDateTime(date, newStartTime)) {
        return;
    }

    // Lock the tree for thread safety
    lock_guard<mutex> lock(treeMtx);

    // Find the date node
    DateNode* dateNode = findDateNode(dateRoot, date);
    if (!dateNode) {
        cout << "No events found for the date: " << date << "\n";
        return;
    }

    // Find the event node
    EventNode* eventNode = findEventNode(dateNode, startTime);
    if (!eventNode) {
        cout << "No event found at " << startTime << " on " << date << "\n";
        return;
    }

    // If start time is changing, check for overlaps
    if (startTime != newStartTime) {
        // Create a temporary copy of the event for overlap checking
        Event tempEvent(date, newStartTime, newEndTime, newDescription);
        
        // Check for overlaps excluding the current event
        bool hasOverlap = false;
        function<void(EventNode*)> checkOverlap = [&](EventNode* node) {
            if (!node) return;
            checkOverlap(node->left);
            
            // Skip comparing with the event being updated
            if (node->event->startTime != startTime) {
                if (newStartTime < node->event->endTime && 
                    newEndTime > node->event->startTime) {
                    hasOverlap = true;
                    cout << "Overlap detected with: [" << node->event->startTime 
                         << " - " << node->event->endTime << "] ---- " 
                         << node->event->description << "\n";
                }
            }
            
            checkOverlap(node->right);
        };
        
        checkOverlap(dateNode->eventRoot);
        
        if (hasOverlap) {
            cout << "Cannot update event, new time overlaps with existing events.\n";
            return;
        }
        
        // Remove the event from heap first
        heap.removeEvent(date, startTime);
        
        // Create new event with updated details
        Event* newEvent = new Event(date, newStartTime, newEndTime, newDescription);
        
        // Remove old event from tree
        dateNode->eventRoot = deleteEventByTime(dateNode->eventRoot, startTime);
        
        // Insert new event into tree
        dateNode->eventRoot = insertEvent(dateNode->eventRoot, newEvent);
        
        // Add to heap
        heap.addEvent(newEvent);
    } else {
        // If only updating description or end time, modify existing event
        eventNode->event->endTime = newEndTime;
        eventNode->event->description = newDescription;
    }
    
    cout << "Event updated successfully!\n";
}
	
	
	    void removeEvent(string date, int startTime){
	        lock_guard<mutex> lock(treeMtx);
	        DateNode* dateNode = findDateNode(dateRoot, date);
	        if (dateNode == nullptr) {
	            cout << "No events found for the date: " << date << "\n";
	            return;
	        }
	        dateNode->eventRoot = deleteEventByTime(dateNode->eventRoot, startTime);
	        heap.removeEvent(date, startTime);
		}
	
	    DateNode* findDateNode(DateNode* root, string date){
	        if (root == nullptr){
	            return nullptr;
	        }
	        if (date < root->date){
	            return findDateNode(root->left, date);
	        }
	        else if (date > root->date){
	            return findDateNode(root->right, date);
	        }
	        else{
	            return root;
	        }
	    }
	
		void searchEventsByTime(EventNode* root, int time){
	        if (root == nullptr){
	            return;
	        }
	        if (time >= root->event->startTime && time <= root->event->endTime){
	            cout << "[" << root->event->startTime << " - " << root->event->endTime 
	                 << "] ---- " << root->event->description << "\n";
	        }
	        if (time < root->event->startTime){
	            searchEventsByTime(root->left, time);
	        }
	        else{
	            searchEventsByTime(root->right, time);
	        }
	    }
	
	    void searchEventsByDate(DateNode* root, string date){
	        if (root == nullptr){
	            cout << "No events found for the date: " << date << "\n";
	            return;
	        }
	        if (date < root->date){
	            searchEventsByDate(root->left, date);
	        }
	        else if (date > root->date){
	            searchEventsByDate(root->right, date);
	        }
	        else{
	            cout << "Events on date: " << date << "\n";
	            inOrderEvents(root->eventRoot);
	        }
	    }
	
	    void searchByDate(string date){
	        lock_guard<mutex> lock(treeMtx);
	        searchEventsByDate(dateRoot, date);
	    }
	
	    void searchByTime(int time){
	        lock_guard<mutex> lock(treeMtx);
	        if (dateRoot == nullptr){
	            cout << "No events found.\n";
	            return;
	        }
	        cout << "Events at time: " << time << "\n";
	        function<void(DateNode*)> searchAllDates = [&](DateNode* node){
	            if (node == nullptr) return;
	            searchAllDates(node->left);
	            searchEventsByTime(node->eventRoot, time);
	            searchAllDates(node->right);	
	        };
	        searchAllDates(dateRoot);
	    }
	
	    void inOrderDates(DateNode* root){
	        if (root == nullptr){
	            return;
	        }
	        inOrderDates(root->left);
	        cout << "Date: " << root->date << "\n";
	        inOrderEvents(root->eventRoot);
	        inOrderDates(root->right);
	    }
	
	    void displayAllEvents(){
	        lock_guard<mutex> lock(treeMtx);
	        if (dateRoot == nullptr){
	            cout << "No events scheduled.\n";
	            return;
	        }
	        cout << "All Events:\n";
	        inOrderDates(dateRoot);
	    }
};

bool isValidDate(const string& date){
    if (date.length() != 10) 
		return false;	

    int year, month, day;
    char dash1, dash2;

    stringstream ss(date);
    ss >> year >> dash1 >> month >> dash2 >> day;

    if (ss.fail() || dash1 != '-' || dash2 != '-') 
		return false;
    if (month < 1 || month > 12) 
		return false;

    const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))){
        if (day > 29) 
			return false;
    }
    else if (day < 1 || day > daysInMonth[month - 1]){
        return false;
    }
    
    return true;
}

bool isValidTime(int time){
    int hours = time / 10000;
    int minutes = (time / 100) % 100;
    int seconds = time % 100;

    if (hours < 0 || hours > 23) 
		return false;
    if (minutes < 0 || minutes > 59) 
		return false;
    if (seconds < 0 || seconds > 59) 
		return false;

    return true;
}

int main(){
    AVLTreeScheduler scheduler;
    
    string date;
    int startTime, endTime;
    string description;
    int newStartTime, newEndTime;
    string newDescription;

    while (true){
        int choice = 0;
        cout << "\n---------------------Dynamic Event Scheduler------------------\n";
        cout << "1. Add Event\n";
        cout << "2. Remove Event\n";
        cout << "3. Search For Events\n";
        cout << "4. Update Event\n";
        cout << "5. Display All Events\n";
        cout << "6. Exit\n";
        cout << "Enter your choice: ";
        
        cin >> choice;
        
        if (cin.fail()){
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }
        
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice){
            case 1:{
                cout << "Enter date (YYYY-MM-DD): ";
                getline(cin, date);
                if (!isValidDate(date)){
                    cout << "Invalid date format.\n";
                    continue;
                }
                
                cout << "Enter start time (HHMMSS): ";
                cin >> startTime;
                if (!isValidTime(startTime)){
                    cout << "Invalid start time.\n";
                    continue;
                }
                
                cout << "Enter end time (HHMMSS): ";
                cin >> endTime;
                if (!isValidTime(endTime)){
                    cout << "Invalid end time.\n";
                    continue;
                }
                
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Enter description: ";
                getline(cin, description);
                
                scheduler.addEvent(date, startTime, endTime, description);
                break;
            }
            case 2:{
                cout << "Enter date (YYYY-MM-DD): ";
                cin >> date;
                cout << "Enter start time (HHMMSS): ";
                cin >> startTime;
                scheduler.removeEvent(date, startTime);
                break;
            }
            case 3:{
                int option;
                cout << "\n1. Search by Date\n2. Search by Time\n";
                cout << "Enter your option: ";
                cin >> option;
                switch (option){
                    case 1:{
                        cout << "Enter the date(YYYY-MM-DD): ";
                        cin >> date;
                        scheduler.searchByDate(date);
                        break;
                    }
                    case 2:{
                        cout << "Enter the time(HHMMSS): ";
                        cin >> startTime;
                        scheduler.searchByTime(startTime);
                        break;
                    }
                    default:{
                        cout << "Invalid Option\n";
                        break;
                    }
                }
                break;
            }
            case 4:{
                while (true){
                    cout << "Enter date (YYYY-MM-DD): ";
                    cin >> date;
                    if (isValidDate(date)){
                        break;
                    }
                    cout << "The Date is invalid or not in the specified format. Enter again.\n";
                }
                while (true){
                    try{
                        cout << "Enter start time of event to update (HHMMSS): ";
                        cin >> startTime;
                        if (isValidTime(startTime)){
                            break;
                        }
                        cout << "The Time is invalid. Enter again.\n";
                    }
                    catch (const invalid_argument& e){
                        cout << "Invalid Input. No Characters allowed\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    }
                }
                while (true){
                    try{
                        cout << "Enter new start time (HHMMSS): ";
                        cin >> newStartTime;
                        if (isValidTime(newStartTime)){
                            break;
                        }
                        cout << "The Time is invalid. Enter again.\n";
                    }
                    catch (const invalid_argument& e){
                        cout << "Invalid Input. No Characters allowed\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    }
                }
                while (true){
                    try {
                        cout << "Enter new end time (HHMMSS): ";
                        cin >> newEndTime;
                        if (isValidTime(newEndTime)){
                            break;
                        }
                        cout << "The Time is invalid. Enter again.\n";
                    }
                    catch (const invalid_argument& e){
                        cout << "Invalid Input. No Characters allowed\n";
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    }
                }
                cin.ignore();
                cout << "Enter new description: ";
                getline(cin, newDescription);
                scheduler.updateEvent(date, startTime, newStartTime, newEndTime, newDescription);
                cout << "Returning to main menu...\n" << flush;
                break;
            }
            case 5:{
                scheduler.displayAllEvents();
                break;
            }
            case 6:{
                cout << "Exiting program. Goodbye!\n";
                return 0;
            }
            default:{
                cout << "Invalid choice! Please try again.\n";
                break;
            }
        }
    }
    return 0;
}