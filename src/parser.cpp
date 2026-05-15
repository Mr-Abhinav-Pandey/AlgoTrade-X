#include <string>
#include <stdexcept>
using namespace std;


// Extracts the value of a key from a flat JSON string
// Example: {"symbol":"BTCUSDT","price":"76739.72"} + key="price" → "76739.72"
string parse_json_value(const string& json, const string& key) {
    string search = "\"" + key + "\"";
    size_t key_pos = json.find(search);
    if (key_pos == string::npos)
        throw runtime_error("Key not found: " + key);

    size_t colon = json.find(':', key_pos);
    if (colon == string::npos)
        throw runtime_error("Malformed JSON");

    size_t val_start = json.find_first_not_of(" \t\n\r", colon + 1);
    if (val_start == string::npos)
        throw runtime_error("No value found");

    // String value
    if (json[val_start] == '"') {
        size_t str_start = val_start + 1;
        size_t str_end = json.find('"', str_start);
        if (str_end == string::npos)
            throw runtime_error("Unterminated string");
        return json.substr(str_start, str_end - str_start);
    }

    // Numeric value
    size_t val_end = json.find_first_of(",}", val_start);
    return json.substr(val_start, val_end - val_start);
}

double parse_double(const string& json, const string& key) {
    return stod(parse_json_value(json, key));
}

string parse_string(const string& json, const string& key) {
    return parse_json_value(json, key);
}
