#include <bits/stdc++.h>
#include "../config/json.hpp" // nlohmann::json single-header
using json = nlohmann::json;
using namespace std;

int main(int argc, char** argv){
	ios::sync_with_stdio(false);
    cin.tie(nullptr);
    string cfg = "../config/config.json";
    if (argc >= 2) cfg = argv[1];
    	cout<<"=============="<<endl;
    	cout<<"== Correcto =="<<endl;
    	cout<<"=============="<<endl;
	
    return 0;
}