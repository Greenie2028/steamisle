#include <iostream>
using std::cout, std::cerr, std::endl, std::cin;

#include <fstream>
using std::ifstream, std::ofstream;

#include <string>
using std::string, std::getline;

#include <vector>
using std::vector;

#include <map>
using std::map;

#include <set>
using std::set;

#include <sstream>
using std::istringstream;

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

struct Game {
    string name;
    string status;
};

void pauseExit(int code) {
    cout << "\nPress Enter to exit...";
    cin.ignore(numeric_limits<std::streamsize>::max(), '\n');
    cin.get();
    exit(code);
}

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

map<string, string> loadConfig(const string& filename) {
    map<string, string> config;
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "ERROR: Failed to open config file: " << filename << endl;
        return config;
    }

    string line;
    while (getline(file, line)) {
        size_t eq = line.find('=');
        if (eq == string::npos) continue;

        string key = line.substr(0, eq);
        string value = line.substr(eq + 1);
        config[key] = value;
    }

    return config;
}

// Validates Steam ID and returns user's display name
string getSteamUsername(const string& user_id, const string& api_key) {
    string url = "https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/?key="
    + api_key + "&steamids=" + user_id;

    string response = fetchURL(url);
    
    if (response == "") {
        return "";
    }

    json data;

    try {
        data = json::parse(response);
    } catch (const json::parse_error& e) {
        cerr << "ERROR: Failed to parse Steam response: " << e.what() << endl;
        return "";
    }
    

    if (data["response"]["players"].empty()) { // Steam ID does not exist
        return "";
    }

    return data["response"]["players"].at(0)["personaname"].get<string>(); // Returns user display name
}

vector<string> getSteamLibrary(const string& user_id, const string& api_key) {
    string url = "https://api.steampowered.com/IPlayerService/GetOwnedGames/v1/?key="
    + api_key + "&steamid=" + user_id + "&include_appinfo=true" // Include game names, not just game ids
    + "&include_played_free_games=true"; // Include free games

    string response = fetchURL(url);
    if (response == "") {
        return {}; // On failure returns an empty vector
    }

    json data;
    try {
        data = json::parse(response);
    } catch (const json::parse_error& e) {
        cerr << "Failed to parse Steam response: " << e.what() << endl;
        return {};
    }

    if(!data["response"].contains("games")) {
        return {};
    }

    vector<string> library;
    for (const auto& game : data["response"]["games"]) {
        library.push_back(game["name"].get<string>());
    }

    return library;
}

vector<string> parseCSVLine(const string& line) { // Has to handle commas within quoted fields
    vector<string> fields;
    string field;
    bool inQuotes = false; // Handling internal quoted fields

    for (int i = 0; i < line.size(); i++) {
        char c = line[i];

        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i+1] == '"') {
                field += '"'; // Add single quote
                i++; // Avoid double quotes
            }
            else {
                inQuotes = !inQuotes;
            }
        }

        else if (c == ',' && !inQuotes) { // Handling stray commas
            fields.push_back(field);
            field.clear();
        }

        else { // All other characters
            field += c;
        }
    }

    fields.push_back(field);
    return fields;
}

vector<vector<string>> parseCSV(const string& csv) {
    vector<vector<string>> rows;
    string line;
    bool inQuotes = false;

    for (char c : csv) {
        if (c == '"') {
            inQuotes = !inQuotes;
            line += c;
        }
        else if (c == '\n' && !inQuotes) { // Handling internal new lines
            if(!line.empty()) {
                rows.push_back(parseCSVLine(line));
                line.clear();
            }
        }
        else {
            line += c;
        } 
    }

    if (!line.empty()) {    // Catches final line if file doesn't end with \n
        rows.push_back(parseCSVLine(line));
    }

    return rows;
}

vector<Game> parseCoreCSV(const string& csv) { // Parsing Core Worlds
    vector<vector<string>> rows = parseCSV(csv);
    vector<Game> games;


    // Row 0 is Note, Row 1 is Headers
    // Data starts on Row 2
    for (int i = 2; i < rows.size(); i++) {
        if (rows[i].empty() || rows[i][0].empty()) continue;
        games.push_back({rows[i][0], "Core-Verified"});
    }

    return games;
}

vector<Game> parsePlayableCSV(const string& csv) {
    vector<vector<string>> rows = parseCSV(csv);
    vector<Game> games;

    // Rows 0-3 are instructions, 4 is headers
    // Data starts on row 5
    for (int i = 5; i < rows.size(); i++) {
        if (rows[i].empty() || rows[i][0].empty()) continue;
        if (rows[i].size() < 2) continue;
        games.push_back({rows[i][0], rows[i][1]}); // Name is at 0, status is at 1
    }

    return games;
}

string toLower(const string& str) {
    string lower = str;
    transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}

string normalizeForMatching(const string& name) {
    vector<string> suffixes = {
        " - Beta APWorld",
        " (Progression Lock)",
        " (TABS)"
    };

    for (const string& suffix : suffixes) { // Removing suffixes for matching
        if (name.size() >= suffix.size() && name.substr(name.size() - suffix.size()) == suffix) {
            return name.substr(0, name.size() - suffix.size());
        }
    }

    return name;
}

int statusPriority(const string& status) {
    if (status == "Core-Verified") return 0;
    if (status == "APWorld Only")  return 1;
    if (status == "Merged")        return 2;
    if (status == "In Review")     return 3;
    if (status == "Stable")        return 4;
    if (status == "Unstable")      return 5;
    if (status == "Broken on Main")        return 6;
    return 7;
}

vector<Game> matchGames(const vector<Game>& archipelago_games, const vector<string>& library) {
    set<string> librarySet;
    for (const string& game : library) {
        librarySet.insert(toLower(game));
    }

    vector<Game> matches;
    for (const Game& game : archipelago_games) {
        string normalized = normalizeForMatching(game.name);
        if (librarySet.count(toLower(normalized))) {
            matches.push_back(game);
        }
    }

    sort(matches.begin(), matches.end(), [](const Game& a, Game& b) {
        int pa = statusPriority(a.status);
        int pb = statusPriority(b.status);
        if (pa != pb) return pa < pb; // Sort by status
        return toLower(a.name) < toLower(b.name); // Same status, sort alphabetically
    });

    return matches;
}

int main(int argc, char* argv[]) {

    bool cmd_line = true;
    if (argc == 1) {
        cmd_line = false;
    }
    else if (argc != 2) {
        cerr << "ERROR: Invalid Usage. Proper Usage: ./steamisle {steamUserId}" << endl;
        pauseExit(1);
    }

    string user_id;
    string core_world_url = "https://docs.google.com/spreadsheets/d/1iuzDTOAvdoNe8Ne8i461qGNucg5OuEoF-Ikqs8aUQZw/edit?gid=1675722515#gid=1675722515";
    string playable_url = "https://docs.google.com/spreadsheets/d/1iuzDTOAvdoNe8Ne8i461qGNucg5OuEoF-Ikqs8aUQZw/edit?gid=58422002#gid=58422002";

    if (cmd_line) {
        user_id = string(argv[1]);
    }
    else {
        cout << "Enter User's Steam Id: ";
        cin >> user_id;
    }

    string formatted_core = buildURL(core_world_url);
    string formatted_playable = buildURL(playable_url);
    
    if (formatted_core == "") {
        cerr << "ERROR: Failed to format core_worlds url." << endl;
        pauseExit(1);
    }

    if (formatted_playable == "") {
        cerr << "ERROR: Failed to format playable_worlds url." << endl;
        pauseExit(1);
    }

    cout << "Fetching Core Worlds CSV..." << endl;
    string csv_core = fetchURL(formatted_core);

    cout << "Fetching Playable Worlds CSV..." << endl;
    string csv_playable = fetchURL(formatted_playable);


    map<string, string> config = loadConfig("steamisle.cfg");
    if (config.find("api_key") == config.end() || config["api_key"] == "") { // TODO: Add in if template was not changed
        cerr << "ERROR: No API key found in steamisle.cfg" << endl;
        cerr << "Get a key at: https://steamcommunity.com/dev/apikey" << endl;
        pauseExit(1);
    }
    string api_key = config["api_key"];

    // Validating Steam User and fetching username
    cout << "Looking up Steam user..." << endl;
    string username = getSteamUsername(user_id, api_key);
    if (username == "") {
        cerr << "ERROR: Steam ID " << user_id << " not found or profile is private." << endl;
        pauseExit(1);
    }

    cout << "Found user: " << username << endl;

    // Fetching Steam Library
    cout << "Fetching Steam library for " << username << "..." << endl;
    vector<string> library = getSteamLibrary(user_id, api_key);
    if (library.empty()) {
        cerr << "ERROR: Could not fetch Steam Library. Profile or game list may be private." << endl;
        pauseExit(1);
    }

    cout << "Found " << library.size() << " games in library." << endl;

    // Parse both CSVs into Game objects
    vector<Game> coreGames = parseCoreCSV(csv_core);
    vector<Game> playableGames = parsePlayableCSV(csv_playable);

    // Combine vectors and cross-reference with library
    vector<Game> allArchipelagoGames;
    for (const Game& g : coreGames) allArchipelagoGames.push_back(g);
    for (const Game& g : playableGames) allArchipelagoGames.push_back(g);

    vector<Game> matches = matchGames(allArchipelagoGames, library);
    cout << "Found " << matches.size() << " Archipelago Games in your library." << endl;

    string outfile_name = username + ".txt";
    outfile_name = validateFile(outfile_name);
    if (outfile_name == "") {
        pauseExit(0);
    }

    ofstream outfile(outfile_name);
    if (!outfile.is_open()) {
        cerr << "ERROR: Could not open output file: " << outfile_name << endl;
        pauseExit(1);
    }

    outfile << "Archipelago Games for " << username << "\n";
    outfile << "Total: " << matches.size() << " games\n";
    outfile << "========================================\n\n";

    string current_status = "";
    for (const Game& g : matches) {
        if (g.status != current_status) {
            current_status = g.status;
            outfile << "\n[" << current_status << "]\n";
        }
        outfile << " " << g.name << "\n";
    }

    outfile.close();
    cout  << "Results saved to " << outfile_name << endl;
    pauseExit(0);
}