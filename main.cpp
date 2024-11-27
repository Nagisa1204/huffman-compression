#include <iostream> 
#include <vector>
#include <queue>
#include <fstream>
#include <unordered_map>
#include <cmath>
#include <bitset>

using namespace std;

// Huffman Tree Node
struct Node {
    uint8_t value;
    int freq;
    Node *left, *right;

    Node(uint8_t val, int frq) : value(val), freq(frq), left(nullptr), right(nullptr) {}
};

// Compare function for priority queue
struct Compare {
    bool operator()(Node* l, Node* r) {
        return l->freq > r->freq;
    }
};

// Global variables
unordered_map<uint8_t, string> huffmanCode;
unordered_map<string, uint8_t> reverseCode;

// Function to build the Huffman Tree and generate codes
Node* buildHuffmanTree(const unordered_map<uint8_t, int>& freqMap) {
    priority_queue<Node*, vector<Node*>, Compare> pq;

    for (auto& pair : freqMap) {
        pq.push(new Node(pair.first, pair.second));
    }

    while (pq.size() > 1) {
        Node *left = pq.top(); pq.pop();
        Node *right = pq.top(); pq.pop();

        Node *node = new Node('\0', left->freq + right->freq);
        node->left = left;
        node->right = right;

        pq.push(node);
    }

    return pq.top();
}

void generateCodes(Node* root, string str) {
    if (!root) return;

    if (root->left == nullptr && root->right == nullptr) {
        huffmanCode[root->value] = str;
        reverseCode[str] = root->value;
    }

    generateCodes(root->left, str + "0");
    generateCodes(root->right, str + "1");
}

// Function to calculate frequency of bytes in the image
unordered_map<uint8_t, int> calculateFrequency(const vector<uint8_t>& data) {
    unordered_map<uint8_t, int> freqMap;
    for (auto byte : data) {
        freqMap[byte]++;
    }
    return freqMap;
}

// Encode the data and save it to a file
void huffmanEncode(const vector<uint8_t>& data, const string& outputFile) {
    auto freqMap = calculateFrequency(data);
    Node* root = buildHuffmanTree(freqMap);

    generateCodes(root, "");

    string encodedString = "";
    for (auto byte : data) {
        encodedString += huffmanCode[byte];
    }

    cout << "Encoded bit stream (first 100 bits): " << encodedString.substr(0, 100) << "..." << endl;

    ofstream out(outputFile, ios::binary);
    int encodedBits = encodedString.size();
    out.write((char*)&encodedBits, sizeof(int));

    for (size_t i = 0; i < encodedString.size(); i += 8) {
        uint8_t byte = 0;
        for (size_t j = 0; j < 8 && i + j < encodedString.size(); j++) {
            byte |= (encodedString[i + j] == '1') << (7 - j);
        }
        out.write((char*)&byte, 1);
    }

    out.close();
}

// Decode the encoded file and reconstruct the original data
vector<uint8_t> huffmanDecode(const string& inputFile) {
    ifstream in(inputFile, ios::binary);
    if (!in) {
        cerr << "Error: Cannot open encoded file!" << endl;
        return {};
    }

    int encodedBits;
    in.read((char*)&encodedBits, sizeof(int));

    string encodedString = "";
    while (true) {
        uint8_t byte;
        in.read((char*)&byte, 1);
        if (in.eof()) break;
        for (int i = 7; i >= 0; i--) {
            if (encodedString.size() < encodedBits)
                encodedString += ((byte >> i) & 1) ? '1' : '0';
        }
    }

    cout << "Decoded bit stream (first 100 bits): " << encodedString.substr(0, 100) << "..." << endl;

    in.close();

    vector<uint8_t> decodedData;
    string temp = "";
    for (char bit : encodedString) {
        temp += bit;
        if (reverseCode.find(temp) != reverseCode.end()) {
            decodedData.push_back(reverseCode[temp]);
            temp = "";
        }
    }

    return decodedData;
}

// Calculate Mean Squared Error (MSE)
double calculateMSE(const vector<uint8_t>& original, const vector<uint8_t>& reconstructed) {
    if (original.size() != reconstructed.size()) {
        cerr << "Error: Data sizes do not match!" << endl;
        return -1; // Return error value
    }

    double mse = 0;
    for (size_t i = 0; i < original.size(); i++) {
        int diff = original[i] - reconstructed[i];
        mse += diff * diff;
    }

    return mse / original.size();
}

// Verify data consistency
bool verifyData(const vector<uint8_t>& original, const vector<uint8_t>& reconstructed) {
    if (original.size() != reconstructed.size()) {
        cerr << "Error: Original and reconstructed data sizes do not match!" << endl;
        return false;
    }

    for (size_t i = 0; i < original.size(); ++i) {
        if (original[i] != reconstructed[i]) {
            cerr << "Data mismatch at index " << i << ": Original = " 
                 << (int)original[i] << ", Reconstructed = " << (int)reconstructed[i] << endl;
            return false;
        }
    }

    return true;
}

// Read BMP file
vector<uint8_t> readBMP(const string& filename, int& width, int& height) {
    ifstream in(filename, ios::binary);
    if (!in) {
        cerr << "Error: Cannot open BMP file!" << endl;
        return {};
    }

    in.seekg(18);
    in.read((char*)&width, 4);
    in.read((char*)&height, 4);

    in.seekg(54);  // BMP header is 54 bytes
    vector<uint8_t> data(width * height);
    in.read((char*)data.data(), data.size());

    in.close();
    return data;
}

// Write BMP file
void writeBMP(const string& filename, const vector<uint8_t>& data, int width, int height) {
    ifstream in("lenna.bmp", ios::binary);
    if (!in) {
        cerr << "Error: Cannot open BMP header file!" << endl;
        return;
    }
    ofstream out(filename, ios::binary);
    if (!out) {
        cerr << "Error: Cannot open output BMP file!" << endl;
        return;
    }

    vector<uint8_t> header(54);
    in.read((char*)header.data(), 54);
    out.write((char*)header.data(), 54);

    out.write((char*)data.data(), data.size());

    in.close();
    out.close();
}

// Main function
int main() {
    string inputFile = "lenna.bmp";
    string encodedFile = "encoded.bin";
    string outputFile = "lenna_r.bmp";

    // 讀取影像資料
    int width, height;
    auto originalData = readBMP(inputFile, width, height);
    if (originalData.empty()) {
        cerr << "Error: Failed to read image data!" << endl;
        return -1;
    }

    // 編碼
    huffmanEncode(originalData, encodedFile);

    // 解碼
    auto decodedData = huffmanDecode(encodedFile);

    // 確認數據一致性
    if (!verifyData(originalData, decodedData)) {
        cerr << "Data verification failed!" << endl;
        return -1;
    }

    // 計算 MSE
    double mse = calculateMSE(originalData, decodedData);
    if (mse != -1) {
        cout << "MSE: " << mse << endl;
    }

    // 計算位元率 (bpp)
    int originalSize = width * height * 8; // 原始影像大小 (bits)
    ifstream encodedStream(encodedFile, ios::binary);
    encodedStream.seekg(0, ios::end);
    int compressedSize = encodedStream.tellg() * 8; // 壓縮後的大小 (bits)
    encodedStream.close();

    double bpp = (double)compressedSize / (width * height);
    cout << "Bits per pixel (bpp): " << bpp << endl;

    // 將解碼後的影像寫入檔案
    writeBMP(outputFile, decodedData, width, height);

    return 0;
}
