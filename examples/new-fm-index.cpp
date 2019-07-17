#include "sdsl/suffix_arrays.hpp"
#include <string>
#include <iostream>
#include <algorithm>
#include <iomanip>

using namespace sdsl;
using namespace std;

int main(int argc, char** argv)
{
    if (argc <  2) {
        cout << "Usage " << argv[0] << " text_file [num_of_errors] [max_locations] [post_context] [pre_context]" << endl;
        cout << "    This program constructs a very compact FM-index" << endl;
        cout << "    which supports count, locate, and extract queries." << endl;
        cout << "    text_file      Original text file." << endl;
        cout << "    num_of_errors  number of errors allowed." << endl;
        cout << "    max_locations  Maximal number of location to report." <<endl;
        cout << "    post_context   Maximal length of the reported post-context." << endl;
        cout << "    pre_context    Maximal length of the pre-context." << endl;

        return 1;
    }
    size_t num_of_errors = 0;
    size_t max_locations = 5;
    size_t post_context = 10; //will show 10 letters after appearance of the pattern P
    size_t pre_context = 10; //will show 10 letters before appearance of the pattern P

    if (argc >= 3){
        num_of_errors = atoi(argv[2]);
    }
    if (argc >= 4) {
        max_locations = atoi(argv[3]);
    }
    if (argc >= 5) {
        post_context = atoi(argv[4]);
    }
    if (argc >= 6) {
        pre_context = atoi(argv[5]);
    }

    //create a file for the reversed text
    string input_file_name = string(argv[1]);
    string file_rev =  input_file_name.substr(0, input_file_name.length()-4)+ "_rev.txt";
    int_vector<8> text;
    load_vector_from_file(text, argv[1], 1);
    size_t n = text.size();
    int_vector<8> text_rev(n);
    for(size_t i=0; i<n; i++){
        text_rev[n-1-i] = text[i];
    }
    char *text2 = (char*)text_rev.data();
    ofstream output_file(file_rev, ofstream::binary);
    output_file.write(text2, n);
    output_file.close();

    //create th fm_idex for the fwd abd bwd text
    csa_wt<wt_hutu<rrr_vector<127> >, 512, 1024> fm_index;
    csa_wt<wt_hutu<rrr_vector<127> >, 512, 1024> fm_index_rev;

    string index_suffix = ".fm9";
    string index_file   = string(argv[1])+index_suffix;
    string index_file_rev   = file_rev + index_suffix;

    if (!load_from_file(fm_index, index_file)) {
        ifstream in(argv[1]);
        if (!in) {
            cout << "ERROR: File " << argv[1] << " does not exist. Exit." << endl;
            return 1;
        }
        cout << "No index "<<index_file<< " located. Building index now." << endl;
        construct(fm_index, argv[1], 1); // generate index
        store_to_file(fm_index, index_file); // save it
    }
    if (!load_from_file(fm_index_rev, index_file_rev)) {
        ifstream in(file_rev);
        if (!in) {
            cout << "ERROR: File " << file_rev << " does not exist. Exit." << endl;
            return 1;
        }
        cout << "No index "<<index_file_rev<< " located. Building index now." << endl;
        construct(fm_index_rev, file_rev, 1); // generate index
        store_to_file(fm_index_rev, index_file_rev); // save it
    }
    cout << "Index construction complete, index requires " << size_in_mega_bytes(fm_index) << " MiB." << endl;
    cout << "Index_rev construction complete, index_rev requires " << size_in_mega_bytes(fm_index_rev) << " MiB." << endl;
    cout << "Input search terms and press Ctrl-D to exit." << endl;
    string prompt = "\e[0;32m>\e[0m ";
    cout << prompt;
    string query;

    while (getline(cin, query)) {
        size_t m  = query.size();
        auto locations = locate_with_errors(fm_index, fm_index_rev, query.begin(), query.begin()+m, m, num_of_errors);
        cout << query << " : " << locations.size() << endl;
        if (locations.size()>0 && max_locations>0) {
            cout << "Location and context of first occurrences: " << endl;
            sort(locations.begin(), locations.end());
            for (size_t i = 0, pre_extract = pre_context, post_extract = post_context; i < min(locations.size(), max_locations); ++i) {
                cout << setw(8) << locations[i] << ": ";
                if (pre_extract > locations[i]) {
                    pre_extract = locations[i];
                }
                if (locations[i]+m+ post_extract > fm_index.size()) {
                    post_extract = fm_index.size()-locations[i]-m;
                }
                auto s   = extract(fm_index, locations[i]-pre_extract, locations[i]+m+ post_extract-1);
                string pre = s.substr(0, pre_extract);
                s = s.substr(pre_extract);
                if (pre.find_last_of('\n') != string::npos) {
                    pre = pre.substr(pre.find_last_of('\n')+1);
                }
                cout << pre;
                cout << "\e[1;31m";
                cout << s.substr(0, m);
                cout << "\e[0m";
                string context = s.substr(m);
                cout << context.substr(0, context.find_first_of('\n')) << endl;
            }
        }
        cout << prompt;
    }
    cout << endl;
}
