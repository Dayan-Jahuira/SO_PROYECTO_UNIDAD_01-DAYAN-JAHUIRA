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

enum class AlgCPU { FCFS, SPN, RR };
enum class AlgMem { FIRST_FIT, BEST_FIT };

string algCPUToStr(AlgCPU a) {
    if (a==AlgCPU::FCFS) return "FCFS";
    if (a==AlgCPU::SPN) return "SPN";
    return "RR";
}
string algMemToStr(AlgMem m) {
    return (m==AlgMem::FIRST_FIT) ? "First-Fit" : "Best-Fit";
}

class Simulator {
public:
    vector<Process> procesos;
    vector<MemReq> solicitudes_mem;
    vector<Block> memoria;
    AlgCPU alg_cpu = AlgCPU::FCFS;
    AlgMem alg_mem = AlgMem::FIRST_FIT;
    int quantum = 4;
    int memoria_size = 1048576;
};

int main(int argc, char** argv){
	ios::sync_with_stdio(false);
    cin.tie(nullptr);
    string cfg = "../config/config.json";
    if (argc >= 2) cfg = argv[1];
    Simulator sim;
    try {
        cout<<"=============="<<endl;
    	cout<<"== Correcto =="<<endl;
    	cout<<"=============="<<endl;
    } catch (exception &e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}