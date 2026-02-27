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

bool fileExists(const string& fileName) { // Checks if given file name already exists
    return fs::exists(fileName);
}

bool validURL(const string& url) { // Compares the start of the url to make sure it's a google sheet
    string valid_URL_key = "https://docs.google.com/spreadsheets/d/";
    string url_substr = url.substr(0, valid_URL_key.size());
    return (url_substr == valid_URL_key);
}

string validateFile(string& file_name) {
    if (fileExists(file_name)) { // File Overwrite Protection
        char overwrite;
        while (overwrite != 'Y' || overwrite != 'y' && overwrite != 'N' || overwrite != 'n') {
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

int main(int argc, char* argv[]) {

    bool cmd_line = true;
    if (argc == 1) {
        cmd_line = false;
    }
    else if (argc != 3) {
        cerr << "ERROR: Invalid Usage. Proper Usage: ./steamisle {steamUserId} {spreadsheet.url}" << endl;
        return 1;
    }

    string user_id;
    string sheet_url;

    if (cmd_line) {
        user_id = string(argv[1]);
        sheet_url = string(argv[2]);
    }
    else {
        cout << "Enter User's Steam Id: ";
        cin >> user_id;
        cout << "Enter Archipelago Google Sheet URL: ";
        cin >> sheet_url;
    }

    if (!validURL(sheet_url)) {
        cerr << "ERROR: " << sheet_url << " is not a valid sheet" << endl;
        cout << "If you believe this is in error, feel free to open an issue ticket at ";
        cout << "https://github.com/Greenie2028/steamisle or send me a message on Discord @ Greenie2028" << endl;
        return 1;
    }

    string outfile_name = (user_id + ".txt");
    outfile_name = validateFile(outfile_name);

    if (outfile_name == "") {
        return 0;
    }

    // Validate Steam User
    
    // Pull Steam User Library Information

    // Pull CSV Data From Sheet

    // Compare Library and Sheet

    // Format and Output Data to Output File

    return 0;
}