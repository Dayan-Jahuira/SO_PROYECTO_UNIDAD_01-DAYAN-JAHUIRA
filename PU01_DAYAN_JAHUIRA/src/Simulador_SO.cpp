#include <bits/stdc++.h>
#include "../config/json.hpp" // nlohmann::json single-header
using json = nlohmann::json;
using namespace std;

struct Process {
    int pid;
    int llegada;
    int servicio;
    int inicio = -1;
    int fin = -1;
    int restante = 0;
    // métricas calculables
    int respuesta() const { return inicio - llegada; }
    int espera() const { return fin - llegada - servicio; }
    int retorno() const { return fin - llegada; }
};

struct MemReq { int pid; int tam; int block_id = -1; };

struct Block {
    int id;
    int inicio;
    int tam;
    bool libre;
    int pid_asignado; // -1 si libre
};

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