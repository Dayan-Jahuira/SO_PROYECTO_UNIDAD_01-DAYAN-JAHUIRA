#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <climits>
#include "../config/json.hpp"
using json = nlohmann::json;
using namespace std;

struct Process {
    int pid;
    int llegada;
    int servicio;
    int inicio = -1;
    int fin = -1;
    int restante = 0;
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
enum class AlgMem { FIRST_FIT, BEST_FIT, WORST_FIT, NEXT_FIT };

string algCPUToStr(AlgCPU a) {
    if (a==AlgCPU::FCFS) return "FCFS";
    if (a==AlgCPU::SPN) return "SPN";
    return "RR";
}
string algMemToStr(AlgMem m) {
    if (m==AlgMem::FIRST_FIT) return "First-Fit";
    if (m==AlgMem::BEST_FIT)  return "Best-Fit";
    if (m==AlgMem::WORST_FIT) return "Worst-Fit";
    return "Next-Fit";
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
    int last_pos = 0; // Para next-fit

    void loadConfig(const string &fname) {
        ifstream f(fname);
        if(!f) throw runtime_error("No se pudo abrir " + fname);
        json j; f >> j;

        string a = j["cpu"]["algoritmo"].get<string>();
        if (a == "FCFS") alg_cpu = AlgCPU::FCFS;
        else if (a == "SPN" || a == "SJF") alg_cpu = AlgCPU::SPN;
        else if (a == "RR") alg_cpu = AlgCPU::RR;
        if (j["cpu"].contains("quantum")) quantum = j["cpu"]["quantum"].get<int>();

        procesos.clear();
        for (auto &p : j["procesos"]) {
            Process pr;
            pr.pid = p["pid"].get<int>();
            pr.llegada = p["llegada"].get<int>();
            pr.servicio = p["servicio"].get<int>();
            pr.restante = pr.servicio;
            procesos.push_back(pr);
        }
        memoria_size = j["memoria"]["tam"].get<int>();
        string me = j["memoria"]["estrategia"].get<string>();
        if (me == "first-fit") alg_mem = AlgMem::FIRST_FIT;
        else if (me == "best-fit") alg_mem = AlgMem::BEST_FIT;
        else if (me == "worst-fit") alg_mem = AlgMem::WORST_FIT;
        else alg_mem = AlgMem::NEXT_FIT;

        solicitudes_mem.clear();
        if (j.contains("solicitudes_mem")) {
            for (auto &m : j["solicitudes_mem"]) {
                MemReq mr;
                mr.pid = m["pid"].get<int>();
                mr.tam = m["tam"].get<int>();
                solicitudes_mem.push_back(mr);
            }
        }

        memoria.clear();
        memoria.push_back(Block{0,0,memoria_size,true,-1});
    }

    int alloc_first_fit(int pid, int tam) {
        for (auto &b : memoria) {
            if (b.libre && b.tam >= tam) {
                return split_block(b, pid, tam);
            }
        }
        return -1;
    }

    int alloc_best_fit(int pid, int tam) {
        int best_idx = -1, best_size = INT_MAX;
        for (int i=0;i<(int)memoria.size();++i) {
            auto &b = memoria[i];
            if (b.libre && b.tam >= tam && b.tam < best_size) {
                best_size = b.tam;
                best_idx = i;
            }
        }
        if (best_idx==-1) return -1;
        return split_block(memoria[best_idx], pid, tam);
    }

    int alloc_worst_fit(int pid, int tam) {
        int worst_idx = -1, worst_size = -1;
        for (int i=0;i<(int)memoria.size();++i) {
            auto &b = memoria[i];
            if (b.libre && b.tam >= tam && b.tam > worst_size) {
                worst_size = b.tam;
                worst_idx = i;
            }
        }
        if (worst_idx==-1) return -1;
        return split_block(memoria[worst_idx], pid, tam);
    }

    int alloc_next_fit(int pid, int tam) {
        int n = memoria.size();
        for (int k=0; k<n; k++) {
            int i = (last_pos + k) % n;
            auto &b = memoria[i];
            if (b.libre && b.tam >= tam) {
                last_pos = i;
                return split_block(b, pid, tam);
            }
        }
        return -1;
    }

    int split_block(Block &b, int pid, int tam) {
        if (b.tam > tam) {
            int newId = memoria.size();
            int remaining = b.tam - tam;
            int allocInicio = b.inicio;
            b.tam = tam;
            b.libre = false;
            b.pid_asignado = pid;
            Block rem{newId, allocInicio + tam, remaining, true, -1};
            memoria.insert(memoria.begin() + (b.id + 1), rem);
            rebuildBlocks();
            return b.id;
        } else {
            b.libre = false;
            b.pid_asignado = pid;
            return b.id;
        }
    }

    void rebuildBlocks() {
        int curId = 0;
        for (auto &b : memoria) b.id = curId++;
    }

    void free_block_by_pid(int pid) {
        for (auto &b : memoria) {
            if (!b.libre && b.pid_asignado == pid) {
                b.libre = true;
                b.pid_asignado = -1;
            }
        }
        vector<Block> nb;
        for (auto &b : memoria) {
            if (!nb.empty() && nb.back().libre && b.libre) {
                nb.back().tam += b.tam;
            } else nb.push_back(b);
        }
        memoria.swap(nb);
        rebuildBlocks();
    }

    void assign_memory_requests() {
        for (auto &req : solicitudes_mem) {
            int bid = -1;
            if (alg_mem == AlgMem::FIRST_FIT) bid = alloc_first_fit(req.pid, req.tam);
            else if (alg_mem == AlgMem::BEST_FIT) bid = alloc_best_fit(req.pid, req.tam);
            else if (alg_mem == AlgMem::WORST_FIT) bid = alloc_worst_fit(req.pid, req.tam);
            else bid = alloc_next_fit(req.pid, req.tam);
            req.block_id = bid;
        }
    }

    // Schedulers
    void run() {
        sort(procesos.begin(), procesos.end(), [](const Process &a, const Process &b) {
            if (a.llegada != b.llegada) return a.llegada < b.llegada;
            return a.pid < b.pid;
        });
        assign_memory_requests();
        if (alg_cpu == AlgCPU::FCFS) run_fcfs();
        else if (alg_cpu == AlgCPU::SPN) run_spn();
        else run_rr();
        print_results();
        print_memory();
    }

    void run_fcfs() {
        int t = 0;
        for (auto &p : procesos) {
            if (t < p.llegada) t = p.llegada;
            p.inicio = t;
            p.fin = t + p.servicio;
            t = p.fin;
        }
    }

    void run_spn() {
        int t = 0;
        vector<Process*> pendientes;
        for (auto &p : procesos) pendientes.push_back(&p);
        sort(pendientes.begin(), pendientes.end(), [](Process* a, Process* b) {
            return a->llegada < b->llegada;
        });
        vector<Process*> ready;
        while (!pendientes.empty() || !ready.empty()) {
            while (!pendientes.empty() && pendientes.front()->llegada <= t) {
                ready.push_back(pendientes.front());
                pendientes.erase(pendientes.begin());
            }
            if (ready.empty()) {
                if (!pendientes.empty()) t = pendientes.front()->llegada;
                continue;
            }
            sort(ready.begin(), ready.end(), [](Process* a, Process* b) {
                return a->servicio < b->servicio;
            });
            Process* cur = ready.front();
            ready.erase(ready.begin());
            if (t < cur->llegada) t = cur->llegada;
            cur->inicio = t;
            cur->fin = t + cur->servicio;
            t = cur->fin;
        }
    }

    void run_rr() {
        int t = 0;
        vector<Process*> pendientes;
        for (auto &p : procesos) pendientes.push_back(&p);
        sort(pendientes.begin(), pendientes.end(), [](Process* a, Process* b) {
            return a->llegada < b->llegada;
        });
        deque<Process*> ready;
        while (!pendientes.empty() || !ready.empty()) {
            while (!pendientes.empty() && pendientes.front()->llegada <= t) {
                ready.push_back(pendientes.front());
                pendientes.erase(pendientes.begin());
            }
            if (ready.empty()) {
                if (!pendientes.empty()) t = pendientes.front()->llegada;
                continue;
            }
            Process* cur = ready.front();
            ready.pop_front();
            if (cur->inicio == -1) cur->inicio = t;
            int use = min(quantum, cur->restante);
            t += use;
            cur->restante -= use;
            while (!pendientes.empty() && pendientes.front()->llegada <= t) {
                ready.push_back(pendientes.front());
                pendientes.erase(pendientes.begin());
            }
            if (cur->restante == 0) cur->fin = t;
            else ready.push_back(cur);
        }
    }

    void print_results() {
        cout << "\nPID | Llegada | Servicio | Inicio | Fin | Respuesta | Espera | Retorno\n";
        cout << "----+---------+----------+--------+-----+-----------+--------+--------\n";
        double sumaResp=0, sumaEsp=0, sumaRet=0;
        int maxFin = 0;
        for (auto &p : procesos) {
            cout << setw(3) << p.pid << " | "
                 << setw(7) << p.llegada << " | "
                 << setw(8) << p.servicio << " | "
                 << setw(6) << p.inicio << " | "
                 << setw(3) << p.fin << " | "
                 << setw(9) << p.respuesta() << " | "
                 << setw(6) << p.espera() << " | "
                 << setw(6) << p.retorno() << "\n";
            sumaResp += p.respuesta();
            sumaEsp += p.espera();
            sumaRet += p.retorno();
            if (p.fin > maxFin) maxFin = p.fin;
        }
        int n = procesos.size();
        cout << fixed << setprecision(2);
        cout << "\nPromedio respuesta \t=\t " << (sumaResp / n) << "\n";
        cout << "Promedio espera     \t=\t " << (sumaEsp / n) << "\n";
        cout << "Promedio retorno   \t=\t " << (sumaRet / n) << "\n";
        double throughput = (double)n / max(1, maxFin);
        cout << "Throughput         \t=\t " << throughput << " procesos/unidad\n";
    }

    void print_memory() {
        cout << "\nMemoria total: " << memoria_size
             << " bytes. Estrategia: " << algMemToStr(alg_mem) << "\n";

        cout << "\nBloques:\n";
        cout << "\n id  | inicio |  tamano  | libre |  pid\n";
        cout << "-----+--------+----------+-------+-----\n";
        for (auto &b : memoria) {
            cout << setw(4) << b.id << " | "
                 << setw(6) << b.inicio << " | "
                 << setw(8) << b.tam << " | "
                 << setw(5) << (b.libre ? "SI" : "NO") << " | "
                 << setw(3) << b.pid_asignado << "\n";
        }

        cout << "\nSolicitudes de memoria:\n";
        cout << "\n pid |    tam   | block_id\n";
        cout << "-----+----------+----------\n";
        for (auto &r : solicitudes_mem) {
            cout << setw(4) << r.pid << " | "
                 << setw(8) << r.tam << " | "
                 << setw(8) << r.block_id << "\n";
        }
    }
};

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    string cfg = "../config/config_first.json";
//    string cfg = "../config/config_next.json";
//    string cfg = "../config/config_best.json";
//    string cfg = "../config/config_worst.json";
    if (argc >= 2) cfg = argv[1];
    Simulator sim;
    try {
        sim.loadConfig(cfg);
    } catch (exception &e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    sim.run();
    return 0;
}
