#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>
using namespace std;

// ------------------- Process Structure -------------------
struct Process {
    int pid;
    int arrival;
    int burst;
    int priority;
    int memory;          // memory requirement
    int waiting = 0;
    int turnaround = 0;
    int remaining = 0;   // for Round Robin
};

// ------------------- Read Processes from File -------------------
vector<Process> readProcesses(string filename) {
    vector<Process> processes;
    ifstream file(filename);
    if (!file) {
        cout << "Error: Could not open file " << filename << endl;
        return processes;
    }

    string header;
    getline(file, header); // skip header line

    Process p;
    while (file >> p.pid >> p.arrival >> p.burst >> p.priority >> p.memory) {
        p.remaining = p.burst;
        processes.push_back(p);
    }
    file.close();
    return processes;
}

// ------------------- Memory Management (First-Fit) -------------------
void FirstFitMemory(vector<Process> &processes, int totalMemory) {
    vector<int> memoryBlocks(totalMemory, 0); // 0 = free, 1 = allocated
    cout << "\nMemory Allocation (First-Fit):\n";
    cout << "Process\tMemoryReq\tStartAddr\tEndAddr\n";

    for (auto &p : processes) {
        int memReq = p.memory;
        int start = -1, count = 0;
        for (int i = 0; i < totalMemory; i++) {
            if (memoryBlocks[i] == 0) {
                if (start == -1) start = i;
                count++;
            } else {
                start = -1;
                count = 0;
            }
            if (count == memReq) break;
        }
        if (count == memReq) {
            for (int i = start; i < start + memReq; i++) memoryBlocks[i] = 1;
            cout << "P" << p.pid << "\t" << memReq << "\t\t" << start << "\t\t" << start + memReq - 1 << endl;
        } else {
            cout << "P" << p.pid << "\t" << memReq << "\t\tAllocation Failed\n";
        }
    }
}

// ------------------- Gantt Chart -------------------
void printGanttChart(vector<int> timeline, vector<int> pids) {
    cout << "\nGantt Chart:\n|";
    for (int pid : pids)
        cout << " P" << pid << " |";
    cout << endl;
    for (int t : timeline)
        cout << t << " ";
    cout << endl;
}

// ------------------- CPU Utilization Helper -------------------
double computeCPUUtil(vector<Process> &processes, int finishTime) {
    double totalBurst = 0;
    for (auto p : processes) totalBurst += p.burst;
    return (totalBurst / finishTime) * 100;
}

// ------------------- FCFS Scheduling -------------------
void FCFS(vector<Process> processes) {
    sort(processes.begin(), processes.end(), [](Process a, Process b) {
        return a.arrival < b.arrival;
    });

    int time = 0;
    vector<int> ganttTimeline;
    vector<int> ganttPids;
    double totalWT = 0, totalTAT = 0;

    for (auto &p : processes) {
        if (time < p.arrival) time = p.arrival;
        p.waiting = time - p.arrival;
        time += p.burst;
        p.turnaround = p.waiting + p.burst;

        ganttTimeline.push_back(time - p.burst); // start
        ganttPids.push_back(p.pid);

        totalWT += p.waiting;
        totalTAT += p.turnaround;
    }
    ganttTimeline.push_back(time); // end

    printGanttChart(ganttTimeline, ganttPids);

    cout << "\nProcess\tWT\tTAT\n";
    for (auto p : processes)
        cout << "P" << p.pid << "\t" << p.waiting << "\t" << p.turnaround << endl;

    cout << fixed << setprecision(2);
    cout << "Average WT: " << totalWT / processes.size() << endl;
    cout << "Average TAT: " << totalTAT / processes.size() << endl;
    cout << "CPU Utilization: " << fixed << setprecision(2)
         << computeCPUUtil(processes, time) << "%" << endl;
}

// ------------------- SJF Scheduling (Non-Preemptive) -------------------
void SJF(vector<Process> processes) {
    int time = 0;
    vector<int> ganttTimeline;
    vector<int> ganttPids;
    double totalWT = 0, totalTAT = 0;
    int n = processes.size();
    vector<bool> completed(n, false);
    int finished = 0;

    while (finished < n) {
        int idx = -1, minBurst = 1e9;
        for (int i = 0; i < n; i++) {
            if (!completed[i] && processes[i].arrival <= time && processes[i].burst < minBurst) {
                minBurst = processes[i].burst;
                idx = i;
            }
        }

        if (idx == -1) { time++; continue; }

        Process &p = processes[idx];
        p.waiting = time - p.arrival;
        time += p.burst;
        p.turnaround = p.waiting + p.burst;
        completed[idx] = true;
        finished++;

        ganttTimeline.push_back(time - p.burst);
        ganttPids.push_back(p.pid);

        totalWT += p.waiting;
        totalTAT += p.turnaround;
    }
    ganttTimeline.push_back(time);

    printGanttChart(ganttTimeline, ganttPids);
    cout << "\nProcess\tWT\tTAT\n";
    for (auto p : processes)
        cout << "P" << p.pid << "\t" << p.waiting << "\t" << p.turnaround << endl;

    cout << fixed << setprecision(2);
    cout << "Average WT: " << totalWT / processes.size() << endl;
    cout << "Average TAT: " << totalTAT / processes.size() << endl;
    cout << "CPU Utilization: " << fixed << setprecision(2)
         << computeCPUUtil(processes, time) << "%" << endl;
}

// ------------------- Round Robin Scheduling -------------------
void RoundRobin(vector<Process> processes, int quantum) {
    int time = 0;
    vector<int> ganttTimeline;
    vector<int> ganttPids;
    double totalWT = 0, totalTAT = 0;
    int n = processes.size();
    int completed = 0;

    while (completed < n) {
        bool executed = false;
        for (auto &p : processes) {
            if (p.remaining > 0 && p.arrival <= time) {
                int runTime = min(quantum, p.remaining);
                ganttTimeline.push_back(time);
                ganttPids.push_back(p.pid);
                time += runTime;
                p.remaining -= runTime;
                if (p.remaining == 0) {
                    p.turnaround = time - p.arrival;
                    p.waiting = p.turnaround - p.burst;
                    totalWT += p.waiting;
                    totalTAT += p.turnaround;
                    completed++;
                }
                executed = true;
            }
        }
        if (!executed) time++;
    }
    ganttTimeline.push_back(time);
    printGanttChart(ganttTimeline, ganttPids);

    cout << "\nProcess\tWT\tTAT\n";
    for (auto p : processes)
        cout << "P" << p.pid << "\t" << p.waiting << "\t" << p.turnaround << endl;

    cout << fixed << setprecision(2);
    cout << "Average WT: " << totalWT / processes.size() << endl;
    cout << "Average TAT: " << totalTAT / processes.size() << endl;
    cout << "CPU Utilization: " << fixed << setprecision(2)
         << computeCPUUtil(processes, time) << "%" << endl;
}

// ------------------- Main Menu -------------------
int main() {
    vector<Process> processes = readProcesses("processes.txt");
    if (processes.empty()) return 0;

    // --- Memory Allocation ---
    cout << "First-Fit Memory Allocation:\n";
    FirstFitMemory(processes, 100); // total memory 100 units

    // --- Algorithm Menu ---
    cout << "\nSelect Scheduling Algorithm:\n";
    cout << "1. FCFS\n2. SJF (Non-Preemptive)\n3. Round Robin\nChoice: ";
    int choice;
    cin >> choice;

    if (choice == 1) FCFS(processes);
    else if (choice == 2) SJF(processes);
    else if (choice == 3) {
        int quantum;
        cout << "Enter time quantum: ";
        cin >> quantum;
        RoundRobin(processes, quantum);
    }
    else cout << "Invalid choice\n";

    return 0;
}
