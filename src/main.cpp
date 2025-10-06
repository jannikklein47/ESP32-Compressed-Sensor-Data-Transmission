#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Esp.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <queue>

// Define constants
const char* WIFI_SSID = "Test Network";
const char* WIFI_PASSWORD = "12345678";
const char* SERVER_URL = "http://10.83.90.181:3001/wab";

// Create a sensor object
Adafruit_MPU6050 mpu;
// Objects to load sensor data into
sensors_event_t a, g, temp;

// Define the data structure to store every value from one sensor reading
struct Reading {
public:
    float timestamp;
    float gX,gY,gZ,aX,aY,aZ,t;
    Reading(float timestamp, float gX, float gY, float gZ, float aX, float aY, float aZ, float t) {
        this->timestamp = timestamp;
        this->gX = gX;
        this->gY = gY;
        this->gZ = gZ;
        this->aX = aX;
        this->aY = aY;
        this->aZ = aZ;
        this->t = t;

    }
};
// Define the data structure to store every important value needed after running Huffman Coding
struct Huffman {
    std::vector<char> chars;
    std::vector<int> freqs;
    std::string base91;
    unsigned int safe_bits;
    unsigned int originalSize;
};

// MPU functions
void initMPU(){
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
}

// Convert an std::vector of foats to a json array in std::string representation
std::string vectorToJSONArray(const std::vector<float> &data) {

    if (data.empty()) return "[],";
    std::string jsonString = "[";
    for (int i = 0; i < data.size(); ++i) {
        
        jsonString += std::to_string(data.data()[i]);
        
        if (i + 1 < data.size()) jsonString += ",";
    }
    jsonString += "],";
    return jsonString;
}


// Networking functions | connect to the test WiFi network
void connectToWiFi() {
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("Connecting to %s", WIFI_SSID);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.printf("\nConnected to %s.\n", WIFI_SSID);
}

// Send Huffman coded data
void sendHTTP(const Huffman &data) {
    //Serial.printf("SendHTTP called\n");
    if (WiFi.status() == WL_CONNECTED) {
        //Serial.println("Sending huffman data...");
        WiFiClient client;
        HTTPClient http;

        if (http.begin(client, SERVER_URL)) {
            http.addHeader("Content-Type", "application/json");

            std::string body;
            body += "{\"chars\":[";
            for (char c : data.chars) {
                body += "\"";
                body += c;
                body += "\",";
            }
            if (!data.chars.empty()) body.pop_back();
            body += "],\"freqs\":[";
            for (int freq : data.freqs) {
                body += std::to_string(freq);
                body += ",";
            }
            if (!data.freqs.empty()) body.pop_back();
            body += "],";
            body += "\"safe_bits\":" + std::to_string(data.safe_bits);
            body += ",\"original_size\":" + std::to_string(data.originalSize);
            body += ",\"base91\":\"";
            body += data.base91;
            body += "\"}";

            int httpCode = http.POST(body.c_str());
            if (httpCode <= 0) Serial.printf("Error: %s\n", http.errorToString(httpCode).c_str());

            http.end();      // free internal resources
            client.stop();   // close socket
        } else {
            Serial.println("Not connected!");
        }
    }
}
// Send a std::string
void sendHTTP(std::string data) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(SERVER_URL);
        http.addHeader("Content-Type", "application/json");

        std::string payload;
        payload += "{\"value\":\"";
        payload += data;
        
        payload += "\"}";

        http.POST(payload.c_str());

        
        http.end();
    }
}
// Send a std::string with information about original data size in bits
void sendHTTP(std::string data, int bits) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(SERVER_URL);
        http.addHeader("Content-Type", "application/json");

        std::string payload;
        payload += "{\"value\":\"";
        payload += data;
        
        payload += "\",\"original_size\":";
        payload += std::to_string(bits);
        payload += "}";

        http.POST(payload.c_str());

        
        http.end();
    }
}


// --- Implementations for lossless compression algorithms ---

// Run-Length Encoding
std::string runLengthEncode(std::string str)
{
    int n = str.length();  		    // Time complexity: O(1)
    std::string temp = ""; 			    // Time complexity: O(1)

    for (int i = 0; i < n; i++) { 	// Time complexity: O(n)

        // Count occurrences of current character
        int count = 1; 			    // Time complexity: O(1)

        // This reduces the outer loop by the exact repetitions it 
        // takes, so the overall time complexity does not change
        while (i < n - 1 && str[i] == str[i + 1]) {
            count++; 				// Time complexity: O(1)
            i++; 				    // Time complexity: O(1)
        }
        // Add character and its count to result
        temp += str[i] + std::to_string(count); // Time complexity: O(1)
    }
    Serial.printf("%i,", ESP.getFreeHeap());
    return temp;					// Time complexity: O(1)
}

// Delta Encoding
std::vector<float> deltaEncode(std::vector<float> values) {

    std::vector<float> result;// Time complexity: O(1)
    result.reserve(values.size());

    // Cannot compress anything if the array consists of one element
    if (values.size()==1) return values;// Time complexity: O(1)


    result.push_back(values.data()[0]);// Time complexity: O(1)
    // Calculate differences for each index and add those to the result
    for (int i = 1; i < values.size(); i++) {   // Time complexity O(n - 1)
        float prev = values.data()[i-1];          // Time complexity: O(1)
        float curr = values.data()[i];            // Time complexity: O(1)
        result.push_back(curr - prev);          // Time complexity: O(1)
    }

    return result;                              // Time complexity: O(1)
}

// Huffman Coding with helper functions

// Data structure for the nodes in the Huffman Tree
class Node {
public:
    char ch;
	int freq;
	Node *left;
    Node *right;
	Node(char c, int f) { // Everything here has a time complexity of O(1)
		ch = c;
        freq = f;
		left = nullptr;
		right = nullptr;
	}
};

// This class ensures that the queue is always sorted by frequency ascending
class Compare {
public:
	bool operator() (Node* a, Node* b) {
		return a->freq > b->freq; // Time complexity: O(1)
	}
};

// Traverse the huffman tree in preorder and output the data of leaf nodes into the unordered map
// The function is recursive, however every node is traversed only once, therefore this scales linear with O(n)
void preOrder(Node* root, std::unordered_map<char, std::string> &codes, std::string curr) {
	if (root == nullptr) return;                            // Time complexity: O(1)

    // Only leaf nodes can contain a valid character for the Huffman codes
	if (root->left == nullptr && root->right==nullptr) {    // Time complexity: O(1)
		codes[root->ch] = curr;                             // Time complexity: O(1)
		return;
	}

	preOrder(root->left, codes, curr + '0');                // Recursive call
	preOrder(root->right, codes, curr + '1');               // Recursive call
}

// Function to delete every Node from memory after generating codes (Nodes are created in memory with "new", meaning we need to manually remove them)
// This function has a time complexity of O(n), every Node is traversed once
void freeTree(Node* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

// returns a map of <symbol, huffman code> for the input char set and frequency set
std::unordered_map<char, std::string> generateHuffmanCodes(std::string s,std::vector<int> freq) {
	
	int n = s.length();                                     // Time complexity: O(1)
    
    // priority queue
	std::priority_queue<Node*, std::vector<Node*>, Compare> pq;
	for (int i=0; i<n; i++) {  // Time complexity: O(n)
		Node* tmp = new Node(s[i],freq[i]); // Time complexity: O(1)
		pq.push(tmp); // Time complexity: O(1)
	}

    // build Huffman tree
	while (pq.size()>=2) { // Executes approximately n-1 times so the time complexity is O(n)
        // every iteration reduces pq by exactly one node (removes two, adds one)
	    
		Node* l = pq.top();                             // Time complexity: O(1)
		pq.pop();                                       // Time complexity: O(1)

		Node* r = pq.top();                             // Time complexity: O(1)
		pq.pop();                                       // Time complexity: O(1)

        // combine the lowest frequency nodes to a new node
		Node* newNode = new Node('$',l->freq + r->freq); // $ = internal node | time complexity: O(1)
		newNode->left = l;                              // Time complexity: O(1)
		newNode->right = r;                             // Time complexity: O(1)

		pq.push(newNode);                               // Time complexity: worst case O(log n)
	}
    // The total complexity of this loop is O(n log n), the inner body takes O(log n), iterated n-1 times.

	Node* root = pq.top();                              // Time complexity: O(1)
	std::unordered_map<char, std::string> codes;
	preOrder(root, codes, "");                          // Time complexity: O(n)
    freeTree(root);                                     
	return codes;
}


// Templates to efficiently sort the calculated frequencies fro lowest to highest, which is needed for the huffman algorithm to work. Switches keys and values from the original map to achieve sorting by frequency
template<typename A, typename B>
std::pair<B,A> flip_pair(const std::pair<A,B> &p)
{
    return std::pair<B,A>(p.second, p.first);
}

template<typename A, typename B>
std::multimap<B,A> flip_map(const std::map<A,B> &src)
{
    std::multimap<B,A> dst;
    std::transform(src.begin(), src.end(), std::inserter(dst, dst.begin()), 
                flip_pair<A,B>);
    return dst;
}

// fill "chars" with every occuring character, and fill "freqs" on every same index with the corresponding frequency, ordered ascending
void generateCharAndFreq(const std::string &input, std::vector<char> &chars, std::vector<int> &freqs) {

    std::map<char, int> freqMap;

    // Count frequencies
    for (char c : input) {
        freqMap[c]++;
    }

    std::multimap<int, char> dst = flip_map(freqMap);
    

    // Copy into vectors
    for (auto &p : dst) {
        chars.push_back(p.second);
        freqs.push_back(p.first);
    }
}

// Takes an std::string and the huffman table and outputs the huffman coded representation
std::string encodeString(const std::string &input, const std::unordered_map<char, std::string> &codes) {
    std::string encoded = "";
    for (char c : input) {
        encoded += codes.at(c);  // look up Huffman code for this char
    }
    return encoded;
}

/**
 * Encodes a string of binary characters ('0' and '1') into a base91 string.
 * To handle a raw bitstream, this function first pads the input binary string 
 * with '0's to make its length a multiple of 8. It then converts this padded 
 * string into a sequence of bytes. Finally, it applies the standard base91 
 * encoding algorithm to the byte sequence. The alphabet is changed where the "
 * character is replaced with ', because we cannot send " in json file safely
 * or it might be interpreted wrongfully
 */
std::string encode_binary_string_to_base91(const std::string &binary_string) {
    const std::string base91_chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$%&()*+,./:;<=>?@[]^_`{|}~'";

    size_t total_bits = binary_string.size();
    size_t pad = (8 - (total_bits % 8)) % 8;            // number of zeros to add
    size_t padded_bits = total_bits + pad;

    std::vector<uint8_t> data;
    data.reserve((padded_bits + 7) / 8);

    for (size_t i = 0; i < padded_bits; i += 8) { // every 8 bits to form a byte
        uint8_t byte = 0;
        for (size_t b = 0; b < 8; ++b) { // every bit
            size_t idx = i + b; // start at the index of i 
            char c = (idx < total_bits) ? binary_string[idx] : '0';
            byte = (byte << 1) | static_cast<uint8_t>(c == '1'); // left shift adds 1, else 0
        }
        data.push_back(byte);
    }

    // Base91 encode | data is a vector containing 8-bit integers
    std::string result;
    result.reserve(data.size() * 2 + 4);

    unsigned int bit_buffer = 0;
    int num_bits = 0;

    for (uint8_t byte : data) {
        // Take next byte, put it into the bit buffer after the already present bits, update the bit count
        bit_buffer |= (static_cast<unsigned int>(byte) << num_bits);
        num_bits += 8;

        while (num_bits > 13) {
            unsigned int value = bit_buffer & 8191; // take the lowest 13 bits
            if (value > 88) { // 13 bits fill only 8192 values, but base91 has 8281 values available. 
                              //Sometimes we need to borrow one bit to not waste space
                result.push_back(base91_chars[value % 91]); // remainder for first character
                result.push_back(base91_chars[value / 91]); // quotient for second character
                bit_buffer >>= 13; // remove the 13 bits that were processed
                num_bits -= 13;
            } else { // We use 14 bits for two characters 
                value = bit_buffer & 16383; // 14 bits
                result.push_back(base91_chars[value % 91]);
                result.push_back(base91_chars[value / 91]);
                bit_buffer >>= 14;
                num_bits -= 14;
            }
        }
    }

    // There may be fewer than 13 bits left
    if (num_bits > 0) {
        result.push_back(base91_chars[bit_buffer % 91]);
        if (num_bits > 7 || bit_buffer > 90) {
            result.push_back(base91_chars[bit_buffer / 91]);
        }
    }

    return result;
}

// This function combines all the steps for huffman coding with base91 encoding
Huffman huffmanEncode(std::string data) {
    std::vector<char> chars;
    std::vector<int> freqs;

    generateCharAndFreq(data, chars, freqs);

    auto codes = generateHuffmanCodes(chars.data(), freqs);
    std::string encoded = encodeString(data, codes);

    Serial.printf("%i,", ESP.getFreeHeap()); // Print free memory to the console AFTER compression


    return {chars, freqs, encode_binary_string_to_base91(encoded), encoded.size(), data.size() * 8};
}

// Collect sensor data, execute compression and transmit via http. Select which compression algorithm to use and how big one packet of data is
void collectSensorData(int packet_size, bool huffman, bool rle, bool delta, bool none) {

    Serial.printf("%i,", ESP.getFreeHeap()); // Print free memory before compression

    int start = millis();

    std::vector<Reading> readings;
    readings.reserve(packet_size);

    for (int i = 0; i < packet_size; i++) {
        mpu.getEvent(&a, &g, &temp);
        readings.emplace_back((float)millis(),
                              g.gyro.x, g.gyro.y, g.gyro.z,
                              a.acceleration.x, a.acceleration.y, a.acceleration.z,
                              temp.temperature);
    }

    if (huffman || rle || none) {
        // Generate a string representation for the readings in JSON
        std::string stringRepr;
        stringRepr.reserve(packet_size * 80);
        stringRepr += "[";

        for (size_t i = 0; i < readings.size(); ++i) {
            const Reading &r = readings.data()[i];
            stringRepr += "[";
            stringRepr += std::to_string(r.timestamp) + ",";
            stringRepr += std::to_string(r.gX) + ",";
            stringRepr += std::to_string(r.gY) + ",";
            stringRepr += std::to_string(r.gZ) + ",";
            stringRepr += std::to_string(r.aX) + ",";
            stringRepr += std::to_string(r.aY) + ",";
            stringRepr += std::to_string(r.aZ) + ",";
            stringRepr += std::to_string(r.t);
            stringRepr += "]";
            if (i + 1 < readings.size()) stringRepr += ",";
        }
        stringRepr += "]";

        if (huffman) {
            Huffman hf = huffmanEncode(stringRepr); // Huffman Code JSON
            Serial.printf("%i", millis() - start); // Print the number of ms the process took
            sendHTTP(hf); // Transmit via HTTP
        }
        if (rle) {
            std::string rlEncoded = runLengthEncode(stringRepr); // Run-Length Encode JSON
            Serial.printf("%i", millis() - start); // Print the number of ms the process took
            sendHTTP(rlEncoded, stringRepr.size() * 8); // Transmit via HTTP, pass original data size in bits
        }
        if (none) {
            Serial.printf("%i,", ESP.getFreeHeap()); // Print free memory (no compression, just after generating sensor readings)
            Serial.printf("%i", millis() - start); // Print the number of ms the process took
            sendHTTP(stringRepr); // Transmit via HTTP
        }
    }

    if (delta) {
        // Create a string representation in JSON so we can compare it afterwards
        std::string stringRepr;
        for (size_t i = 0; i < readings.size(); ++i) {
            const Reading &r = readings.data()[i];
            stringRepr += "[";
            stringRepr += std::to_string(r.timestamp) + ",";
            stringRepr += std::to_string(r.gX) + ",";
            stringRepr += std::to_string(r.gY) + ",";
            stringRepr += std::to_string(r.gZ) + ",";
            stringRepr += std::to_string(r.aX) + ",";
            stringRepr += std::to_string(r.aY) + ",";
            stringRepr += std::to_string(r.aZ) + ",";
            stringRepr += std::to_string(r.t);
            stringRepr += "]";
            if (i + 1 < readings.size()) stringRepr += ",";
        }
        stringRepr += "]";

        // Change the dimensions of the sensor readings -> dont calculate differences in rows (between different sensors)
        //              -> But in columns (differences in readings by the same sensor)
        std::vector<std::vector<float>> deltaEncArr(8);
        deltaEncArr.reserve(8);
        
        for (const Reading r : readings) {
            deltaEncArr.data()[0].push_back(r.timestamp);
            deltaEncArr.data()[1].push_back(r.gX);
            deltaEncArr.data()[2].push_back(r.gY);
            deltaEncArr.data()[3].push_back(r.gZ);
            deltaEncArr.data()[4].push_back(r.aX);
            deltaEncArr.data()[5].push_back(r.aY);
            deltaEncArr.data()[6].push_back(r.aZ);
            deltaEncArr.data()[7].push_back(r.t);
        }

        auto timestamp = deltaEncode(deltaEncArr.data()[0]);
        auto gX = deltaEncode(deltaEncArr.data()[1]);
        auto gY = deltaEncode(deltaEncArr.data()[2]);
        auto gZ = deltaEncode(deltaEncArr.data()[3]);
        auto aX = deltaEncode(deltaEncArr.data()[4]);
        auto aY = deltaEncode(deltaEncArr.data()[5]);
        auto aZ = deltaEncode(deltaEncArr.data()[6]);
        auto t  = deltaEncode(deltaEncArr.data()[7]);

        std::string jsonString = "[";
        jsonString += vectorToJSONArray(timestamp);
        jsonString += vectorToJSONArray(gX);
        jsonString += vectorToJSONArray(gY);
        jsonString += vectorToJSONArray(gZ);
        jsonString += vectorToJSONArray(aX);
        jsonString += vectorToJSONArray(aY);
        jsonString += vectorToJSONArray(aZ);
        jsonString += vectorToJSONArray(t);
        if (!jsonString.empty() && jsonString.back() == ',') jsonString.pop_back();
        jsonString += "]";

        Serial.printf("%i,%i", ESP.getFreeHeap(), millis() - start); // Print free memory after compression and how long the process took
        sendHTTP(jsonString, stringRepr.size() * 8); // Transmit via HTTP, pass original data size in bits
    }

}

// Main ESP32 functions
void setup() {
    Serial.begin(115200); // Enable reading from the serial console at 115200 BAUD rate
    connectToWiFi();
    initMPU();
    Serial.println("time,mem (start),mem (end),lat"); // for csv purposes, there are the column heads
}

void loop() {
    Serial.printf("%i,", millis()); // Start each log entry with a timestamp
    collectSensorData(100, false, false, false, true); // Main process -> 100 readings per packet, Huffman Coding is selected
    Serial.print("\n"); // log -> new line
}

/* --- use this code to get available memory ---
void setup() {
    Serial.begin(115200);
    Serial.printf("%i\n", ESP.getHeapSize());
}
void loop() {
    Serial.printf("%i, %i\n", ESP.getFreeHeap(), ESP.getHeapSize());
    delay(50);
}*/