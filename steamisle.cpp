#include <iostream>
using std::cout, std::cerr, std::endl, std::cin;

#include <fstream>
using std::ifstream, std::ofstream;

#include <string>
using std::string, std::getline;

#include <filesystem>
namespace fs = std::filesystem;

#include <limits>
using std::numeric_limits;

#include <chrono>
using std::chrono::milliseconds;

#include <thread>
using std::this_thread::sleep_for;

#include <curl/curl.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool fileExists(const string& fileName) { // Checks if given file name already exists
    return fs::exists(fileName);
}

bool validURL(const string& url) { // Compares the start of the url to make sure it's a google sheet
    string valid_URL_key = "https://docs.google.com/spreadsheets/d/";
    string url_substr = url.substr(0, valid_URL_key.size());
    return (url_substr == valid_URL_key);
}

string extractSheetId(const string& sheet_url) { // Extracting unique identifier
    string marker = "/d/";
    size_t id_start = sheet_url.find(marker);
    if (id_start == string::npos) {
        return "";
    }

    id_start += marker.size();
    size_t id_end = sheet_url.find("/", id_start);

    if (id_end == string::npos) {
        return "";
    }

    return sheet_url.substr(id_start, id_end - id_start);
}

string extractGid(const string& sheet_url) { // Extracting Table gid.
    string marker = "/edit?gid=";
    size_t gid_start = sheet_url.find(marker);
    if (gid_start == string::npos) {
        return "";
    }
    gid_start += marker.size();
    size_t gid_end = sheet_url.find("#", gid_start);

    if (gid_end == string::npos) {
        return "";
    }

    return sheet_url.substr(gid_start, gid_end - gid_start);
}

string buildURL(const string& base_url) {
    if (!validURL(base_url)) {
        cerr << "ERROR: " << base_url << " is not a valid sheet url." << endl;
        cout << "If you believe this is in error, feel free to open an issue ticket at ";
        cout << "https://github.com/Greenie2028/steamisle or send me a message on Discord @ Greenie2028." << endl;
        return "";
    }
    string sheet_id = extractSheetId(base_url);
    if (sheet_id == "") {
        cerr << "ERROR: Failed to extract sheet identifer." << endl;
        return "";
    }
    string gid = extractGid(base_url);
    if (gid == "") {
        cerr << "ERROR: Failed to extract gid." << endl;
        return "";
    }
    return "https://docs.google.com/spreadsheets/d/" + 
    sheet_id + "/export?format=csv&gid=" + gid;
}

string validateFile(string& file_name) {
    if (fileExists(file_name)) { // File Overwrite Protection
        char overwrite = 0;
        while (true) {
            cout << "ERROR: " << file_name << " already exists. Do you want to overwrite " << file_name << "? (y/n): ";
            cin >> overwrite;
            cin.ignore(numeric_limits<std::streamsize>::max(), '\n');
            if (overwrite == 'Y' || overwrite == 'y') {
                cout << "Overwriting " << file_name << endl;
                return file_name;
            }

            else if (overwrite == 'N' || overwrite == 'n') {
                while (true) {
                    cout << "Would you like to rename the output file? (y/n): ";
                    cin >> overwrite;
                    cin.ignore(numeric_limits<std::streamsize>::max(), '\n');
                    if (overwrite == 'y' || overwrite == 'Y') {
                        cout << "Enter new file name w/o file extention: ";
                        cin >> file_name;
                        return file_name + ".txt";
                    }

                    else if (overwrite == 'n' || overwrite == 'N') {
                        cout << "Aborting Program..." << endl;
                        sleep_for(milliseconds(500));
                        return "";
                    }

                    else {
                        cerr << "ERROR: Unrecognized command: " << overwrite << endl;
                    }
                }
            }

            else {
                cerr << "ERROR: Unrecognized Command: " << overwrite << endl;
            }
        }

    }

    return file_name;
}

size_t writeCallback(char* data, size_t itemSize, size_t itemCount, string* output) {
    size_t totalBytes = itemSize * itemCount;
    output->append(data, totalBytes);
    return totalBytes;
}

string fetchURL(const string& url) {
    CURL* curl = curl_easy_init();
    
    if (!curl) {
        cerr << "ERROR: Failed to initialize curl" << endl;
        return "";
    }
    
    string response;

    // Setting curl settings
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode result = curl_easy_perform(curl);

    if (result != CURLE_OK) { // Check to make sure no errors were thrown.
        cerr << "ERROR: curl request failed: " << curl_easy_strerror(result) << endl;
        curl_easy_cleanup(curl);
        return "";
    }

    curl_easy_cleanup(curl);
    return response;
}

int main(int argc, char* argv[]) {

    bool cmd_line = true;
    if (argc == 1) {
        cmd_line = false;
    }
    else if (argc != 4) {
        cerr << "ERROR: Invalid Usage. Proper Usage: ./steamisle {steamUserId} {playableWorldsURL} {coreWorldsURL}" << endl;
        return 1;
    }

    string user_id;
    string username; // This will be initialized as the Steam Username used for filenaming
    string core_world_url;
    string playable_url;

    if (cmd_line) {
        user_id = string(argv[1]);
        playable_url = string(argv[2]);
        core_world_url = string(argv[3]);
    }
    else {
        cout << "Enter User's Steam Id: ";
        cin >> user_id;
        cout << "Enter Archipelago Core Worlds Google Sheet URL: ";
        cin >> core_world_url;
        cout << "Enter Archipelago Playable Worlds Google Sheet URL: ";
        cin >> playable_url;
    }

    string formatted_core = buildURL(core_world_url);
    string formatted_playable = buildURL(playable_url);
    
    if (formatted_core == "") {
        cerr << "ERROR: Failed to format core_worlds url." << endl;
        return 1;
    }

    if (formatted_playable == "") {
        cerr << "ERROR: Failed to format playable_worlds url." << endl;
        return 1;
    }

    cout << "Fetching Core Worlds CSV..." << endl;
    string csv_core = fetchURL(formatted_core);

    cout << "Fetching Playable Worlds CSV..." << endl;
    string csv_playable = fetchURL(formatted_playable);


    /* TODO: Grab username from steam data
    string outfile_name = (user_id + ".txt");
    outfile_name = validateFile(outfile_name);

    if (outfile_name == "") {
        return 0;
    }
    */
    // Validate Steam User
    
    // Pull Steam User Library Information

    // Pull CSV Data From Sheet

    // Compare Library and Sheet

    // Format and Output Data to Output File

    return 0;
}