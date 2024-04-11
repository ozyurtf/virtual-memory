#include <vector> 
#include <fstream> 
#include <iostream>
#include <utility>
#include <sstream>
#include <deque>
#include <limits>
#include <unistd.h>
#include <iomanip>
#include <map>

using namespace std;

string filepath;
string rfilepath;
char pager = 'a';
int NumFrames = 4;
const int NumVPages = 64;
vector<int> randVals;

bool printOutputs = false;
bool printPT = false;
bool printF = false;
bool printSummary = false;
bool printAging = false;

int ofs = 1;
int lastRefReset = 0;

vector<int> readRandom(string rfilePath) {
    ifstream file(rfilePath);
    string line;
    vector<int> randVals;
    while (getline(file, line)) {
        randVals.push_back(stoi(line));
    }
    return randVals;
};

int assignRandom(vector<int> randVals) {
    if (ofs > randVals[0]) { 
        ofs = 1;
    }
    int randVal = (randVals[ofs] % NumFrames);
    ofs++;
    return randVal;
};

struct Statistics { 
    unsigned int pid = 0; 
    unsigned long unmaps = 0;
    unsigned long maps = 0;
    unsigned long ins = 0; 
    unsigned long outs = 0;
    unsigned long fins = 0;
    unsigned long fouts = 0; 
    unsigned long zeros = 0;
    unsigned long segv = 0;
    unsigned long segprot = 0; 
};

struct TotalCost { 
    unsigned long currentInst = 0;
    unsigned long contextSwitches = 0;
    unsigned long processExits = 0;
    unsigned long long cost = 0;
    unsigned long sizePTE = 0;
};

TotalCost totalcost;

struct VMA { 
    unsigned int startVPage: 6; 
    unsigned int endVPage: 6;
    unsigned int writeProtect: 1;
    unsigned int fileMapped: 1;     
}; 

struct PageTableEntry {      
    unsigned int present : 1;          
    unsigned int referenced: 1;        
    unsigned int modified : 1;   
    unsigned int writeProtect : 1;       
    unsigned int out : 1;
    unsigned int fout : 1;
    unsigned int partofVMA: 1;    
    unsigned int fileMapped: 1;    
    unsigned int vpage: 6;         
    unsigned int frameNum : 7;         

    PageTableEntry() {
        present = 0;
        referenced = 0;
        modified = 0;
        writeProtect = 0;
        out = 0;
        fout = 0;
        partofVMA = 0;
        fileMapped = 0;
        vpage = 0;
        frameNum = 0;
    }                   
};

 struct Frame { 
    unsigned int frameNum: 7;
    unsigned int pid: 4;                   
    unsigned int vpage: 6;    
    unsigned int age;
    unsigned int lastuse;
    unsigned int allocationStatus = 0;
};

struct Process {
    vector<PageTableEntry> PageTable;
    vector<VMA> vmas;   
 };

vector<Frame> FrameTable;
deque<Frame*> FreeFramePool;
vector<Process> ProcessTable;
vector<PageTableEntry> SwapSpace;
vector<Statistics> StatTable;

void unmap(Frame* victimFrame) { 
    int pid = victimFrame->pid;
    int vpage = victimFrame->vpage;  
    PageTableEntry& PTE = ProcessTable[pid].PageTable[vpage];  
    stringstream outputs;


    outputs << " "
            << "UNMAP"
            << " "
            << pid
            << ":"
            << vpage
            << endl;
                    
    StatTable[pid].unmaps++;                 
        
    if (PTE.modified) { 
        if (!PTE.fileMapped) { 
            outputs << " OUT" << endl;
            PTE.out = 1;
            StatTable[pid].outs++; 
        }
            
        else if (PTE.fileMapped) { 
            outputs << " FOUT" << endl;
            PTE.fout = 1;   
            StatTable[pid].fouts++;
        }
        PTE.modified = 0;
    }

    PTE.present = 0;    
    victimFrame->allocationStatus = 0;

    if (printOutputs) { 
        printf("%s", outputs.str().c_str());
    }
};

struct Pager {
    virtual Frame* selectVictimFrame() = 0; 
};

struct RANDOM: public Pager { 
    unsigned int hand: 7;  

    Frame* selectVictimFrame() {
        hand = assignRandom(randVals);
        Frame* victimFrame = &FrameTable[hand];
        unmap(victimFrame);
        return victimFrame;
    }
};

struct FIFO: public Pager {  
    
    unsigned int hand: 7;   

    FIFO() { 
        hand = 0;
    }

    Frame* selectVictimFrame() {
        stringstream aselect;



        Frame* victimFrame = &FrameTable[hand];

        if (printAging) { 
            aselect << "ASELECT" 
                    << " "
                    << hand
                    << endl;            

            printf("%s", aselect.str().c_str());
        }

        hand = (hand + 1) % NumFrames; 

        unmap(victimFrame);
        return victimFrame;
    }
};

struct CLOCK: public Pager {  
    unsigned int hand;

    CLOCK() { 
        hand = 0;
    }

    Frame* selectVictimFrame() { 
        int inspect = 0;
        bool stop = false;

        int victimFrameInd = 0;
        stringstream aselect;

        Frame* victimFrame = nullptr;
        Frame* frame = nullptr;
        Process* process = nullptr;
        PageTableEntry* PTE = nullptr;


        if (printAging) { 
            aselect << "ASELECT" 
                    << " "
                    << hand   
                    << " ";  
        }  

        while (!stop) {

            frame = &FrameTable[hand];
            process = &ProcessTable[frame->pid];
            PTE = &process->PageTable[frame->vpage];

            if (!PTE->referenced) { 
                victimFrameInd = hand;
                stop = true;
            }

            else if (PTE->referenced) { 
                PTE->referenced = 0;
            }

            inspect++;

            hand = (hand + 1) % NumFrames;           
        }

        if (printAging) { 
            aselect << inspect << endl;
            printf("%s", aselect.str().c_str());
        }

        victimFrame = &FrameTable[victimFrameInd];
        unmap(victimFrame);
        return victimFrame;
    }     
};

struct ESC: public Pager {  
    unsigned int hand: 7;  

    ESC() { 
        hand = 0;
    }    
    
    Frame* selectVictimFrame() {

        int start = hand;           
        bool stop = false;
        int victimFrameInd = 0;
        int classNum;
        int inspect = 0;
        bool reset = totalcost.currentInst - lastRefReset >= 47;
        stringstream aselect;
        
        deque<int> class0;
        deque<int> class1;
        deque<int> class2;
        deque<int> class3;

        Frame* victimFrame = nullptr;
        Frame* frame = nullptr;
        Process* process = nullptr;
        PageTableEntry* PTE = nullptr;

        if (printAging) { 
            aselect << "ASELECT" 
                    << ":"
                    << " "
                    << "hand"
                    << "="
                    << setw(2)
                    << hand   
                    << setw(2)
                    << reset
                    << setw(2)
                    << "|"
                    << " ";
        }
      
        while (!stop) {
                                    
            frame = &FrameTable[hand];
            process = &ProcessTable[frame->pid];
            PTE = &process->PageTable[frame->vpage];
                      
            classNum = 2 * PTE->referenced + PTE->modified;

            if (classNum == 0 && class0.empty()) { 
                class0.push_back(hand);

                if (!reset) { 
                    stop = true;
                }
            }

            else if (classNum == 1 && class0.empty() && class1.empty()) { 
                class1.push_back(hand);
            }

            else if (classNum == 2 && class0.empty() && class1.empty() && class2.empty()) { 
                class2.push_back(hand);
            }

            else if (classNum == 3 && class0.empty() && class1.empty() && class2.empty() && class3.empty()) { 
                class3.push_back(hand);
            }

            if (reset) { 
                PTE->referenced = 0;
            }                 

            inspect++;

            hand = (hand + 1) % NumFrames;
            
            if (hand == start) { 
                stop = true;
            }                                  
        }

        if (reset) { 
            lastRefReset = totalcost.currentInst + 1;
        }
        
        if (!class0.empty()) { 
            victimFrameInd = class0.front();
            class0.pop_front();

            if (printAging) { 
                aselect << "0 " << setw(2);
            }
        }
        else if (class0.empty() && !class1.empty()) { 
            victimFrameInd = class1.front();
            class1.pop_front(); 

            if (printAging) { 
                aselect << "1 " << setw(2);
            }
        }

        else if (class0.empty() && class1.empty() && !class2.empty()) { 
            victimFrameInd = class2.front();
            class2.pop_front(); 

            if (printAging) { 
                aselect << "2 " << setw(2);           
            }
        }  

        else if (class0.empty() && class1.empty() && class2.empty() && !class3.empty()) { 
            victimFrameInd = class3.front();
            class3.pop_front();  

            if (printAging) { 
                aselect << "3 " << setw(2);         
            }
        }


        if (printAging) { 
            aselect << victimFrameInd 
                    << " "
                    << setw(2)
                    << inspect;            
            printf("%s\n", aselect.str().c_str());               
        }

        victimFrame = &FrameTable[victimFrameInd]; 
        hand = (victimFrameInd + 1) % NumFrames;
                   
        unmap(victimFrame);
        return victimFrame;
    }        
};

struct AGING: public Pager {  
    
    unsigned int hand: 7;  

    AGING() { 
        hand = 0;
    }

    Frame* selectVictimFrame() {
        
        int start = hand;            
        bool stop = false;
        int victimFrameInd = 0;
        int minAge = numeric_limits<unsigned int>::max();
        int end;

        stringstream aselectBefore;
        stringstream aselectDuring;
        stringstream aselectAfter;

        if (printAging) { 
            aselectBefore << "ASELECT"
                          << " "
                          << start 
                          << "-";  
        }  
        
        Frame* victimFrame = nullptr;
        Frame* frame = nullptr;
        Process* process = nullptr;
        PageTableEntry* PTE = nullptr;

        while (!stop) {
                                    
            frame = &FrameTable[hand];
            process = &ProcessTable[frame->pid];
            PTE = &process->PageTable[frame->vpage];
            frame->age = frame->age >> 1;

            if (PTE->referenced) { 
                frame->age = frame->age | 0x80000000;
                PTE->referenced = 0;
            }

            if (frame->age < minAge) { 
                minAge = frame->age;
                victimFrameInd = hand;
            } 

            if (printAging) { 
                aselectDuring << hand
                            << ":"
                            << hex 
                            << frame->age
                            << dec
                            << " ";  
            }          

            end = hand;   
            hand = (hand + 1) % NumFrames;

            if (start == hand) { 
                stop = true;
            }
               
        };

        victimFrame = &FrameTable[victimFrameInd];
        hand = (victimFrameInd + 1) % NumFrames;

        if (printAging)  { 
            aselectBefore << end << " ";
            aselectAfter << victimFrameInd;                

            printf("%s | %s | %s\n", aselectBefore.str().c_str(), 
                                     aselectDuring.str().c_str(), 
                                     aselectAfter.str().c_str());
        }
        
        unmap(victimFrame);
        return victimFrame;
    }        
};

struct WORKSET: public Pager {  
    unsigned int hand: 7;  

    WORKSET() { 
        hand = 0;
    }

    Frame* selectVictimFrame() {

        int start = hand;           
        bool stop = false;
        int victimFrameInd = 0;
        int inspect = 0; 
        int end = 0;
        int age;
        bool selected = false;

        deque<unsigned int> handVector;
        deque<unsigned int> lastUseVector;

        deque<unsigned int> handVectorR;
        deque<unsigned int> lastUseVectorR;

        stringstream aselectBefore;
        stringstream aselectDuring;
        stringstream aselectAfter;

        if (printAging) { 
            aselectBefore << "ASELECT"
                        << " "
                        << start 
                        << "-";       
        }    

        Frame* victimFrame = nullptr;
        Frame* frame = nullptr;
        Process* process = nullptr;
        PageTableEntry* PTE = nullptr;

        while (!stop) {

            inspect++;
                                    
            frame = &FrameTable[hand];
            process = &ProcessTable[frame->pid];
            PTE = &process->PageTable[frame->vpage];
            age = totalcost.currentInst - frame->lastuse;

            if (printAging) { 
                aselectDuring << hand 
                            << "("                  
                            << PTE->referenced
                            << " "
                            << frame->pid
                            << ":"
                            << frame->vpage
                            << " "
                            << frame->lastuse
                            << ")"
                            << " ";  
            }
                           
                     
            if (PTE->referenced) { 
                handVectorR.push_back(hand);
                lastUseVectorR.push_back(frame->lastuse);  

                PTE->referenced = 0;
                frame->lastuse = totalcost.currentInst;                                   
            }

            else if (!PTE->referenced) {             

                handVector.push_back(hand);
                lastUseVector.push_back(frame->lastuse);  

                end = hand;
                
                if (age > 49) { 
                    victimFrameInd = hand;
                    selected = true;

                    if (printAging) { 
                        aselectDuring << "STOP" 
                                    << "(" 
                                    << inspect
                                    << ")" 
                                    << " ";
                    }

                    break;
                }
            }

            end = hand;

            hand = (hand + 1) % NumFrames;

            if (hand == start) { 
                stop = true;
            }                
        }

  
        if (!selected) { 

            int minLastUse = numeric_limits<unsigned int>::max();

            if (!handVector.empty()) {
                for (int i = 0; i < handVector.size(); i++) {
                    if (lastUseVector[i] < minLastUse) {
                        victimFrameInd = handVector[i];
                        minLastUse = lastUseVector[i];
                    }
                }
            }

            else if (handVector.empty()) { 
                victimFrameInd = handVectorR[0];               
            }  
        }

        hand = (victimFrameInd + 1) % NumFrames;      

        if (printAging)  { 
            aselectBefore << end;
            aselectAfter << victimFrameInd;   
            printf("%s | %s | %s\n", aselectBefore.str().c_str(), 
                                     aselectDuring.str().c_str(), 
                                     aselectAfter.str().c_str());
        }

        victimFrame = &FrameTable[victimFrameInd];                  
        unmap(victimFrame);
        return victimFrame;
    }        
};

FIFO FIFOPager;
RANDOM RANDOMPager;
CLOCK CLOCKPager;
ESC ESCPager;
AGING AGINGPager;
WORKSET WORKSETPager;

int allocateFrame(char pager) { 

    Frame* frame;

    if (!FreeFramePool.empty()) { 
        frame = FreeFramePool.front();
        FreeFramePool.pop_front(); 
    }
    
    else if (FreeFramePool.empty()) {  
        if (pager == 'F' || pager == 'f') { 
            frame = FIFOPager.selectVictimFrame();
        }

        else if (pager == 'R' || pager == 'r') { 
            frame = RANDOMPager.selectVictimFrame();
        }

        else if (pager == 'C' || pager == 'c') { 
            frame = CLOCKPager.selectVictimFrame();
        }

        else if (pager == 'E' || pager == 'e') { 
            frame = ESCPager.selectVictimFrame();
        }

        else if (pager == 'A' || pager == 'a') { 
            frame = AGINGPager.selectVictimFrame();
        }

        else if (pager == 'W' || pager == 'w') { 
            frame = WORKSETPager.selectVictimFrame();
        }
    }

    return frame->frameNum;
}

void mapPTE(int vpage, int pid, char pager) { 

    PageTableEntry* PTE = &ProcessTable[pid].PageTable[vpage];
    stringstream outputs;
    
    if (!PTE->present) { 

        if (!PTE->partofVMA) { 
            outputs << " SEGV" << endl;
            StatTable[pid].segv++;
        }

        else if (PTE->partofVMA) { 
            int frameNum = allocateFrame(pager);

            Frame* frame = &FrameTable[frameNum];
                
            frame->pid = pid;
            frame->vpage = PTE->vpage;
            frame->age = 0;
            frame->lastuse = totalcost.currentInst;
            frame->allocationStatus = 1;

            PTE->frameNum = frame->frameNum;
            PTE->present = 1;    

            if (PTE->out) { 
                outputs << " IN" << endl; 
                StatTable[pid].ins++;
            }            

            else if (PTE->fileMapped) { 
                outputs << " FIN" << endl;  
                StatTable[pid].fins++;     
            }
            
            else if (!PTE->out && !PTE->fout && !PTE->fileMapped) { 
                outputs << " ZERO" << endl;
                StatTable[pid].zeros++;
            }

            outputs << " " 
                    << "MAP" 
                    << " "
                    << frame->frameNum
                    << endl;   

            StatTable[pid].maps++;        
        }
    }

    if (printOutputs) { 
        printf("%s", outputs.str().c_str());
    }
};

int main(int argc, char* argv[]) {    

    int opt;
    string options; 
    
    while ((opt = getopt(argc, argv, "f:a:o:")) != -1) { 
        switch(opt) { 
            case 'f': 
                NumFrames = atoi(optarg); 
                break;
            case 'a': 
                pager = optarg[0]; 
                break;
            case 'o': 
                options = optarg;  
                break;                                   
        }
    }

    filepath = argv[optind];
    rfilepath = argv[optind + 1];
    randVals = readRandom(rfilepath);  

    for (char& option: options) { 
        switch (option) { 
            case 'O': 
                printOutputs = true;
                break;
            case 'P': 
                printPT = true;
                break;
            case 'F': 
                printF = true;
                break;
            case 'S': 
                printSummary = true;
                break;
            case 'a': 
                printAging = true;
                break;             
        }   
    }
    ifstream file(filepath);
    
    int numProcess = 0;
    int processNum = 0;
    
    int instructionNum = 0;
    int currentPID;

    int numVMAs = 0;
    int VMANum = 0;

    bool lineSkip = false;
    bool processStart = false;
    bool VMAStart = false;    
    bool instructionStart = false;

    Process* currentProcess;
    Statistics* currentStats;
    string line; 

    totalcost.sizePTE = sizeof(PageTableEntry);

    for (int frameNum = 0; frameNum < NumFrames; frameNum++) { 
        Frame* freeFrame = new Frame();
        freeFrame->frameNum = frameNum;
        FreeFramePool.push_back(freeFrame);
        FrameTable.push_back(*freeFrame);
    }   

    while (getline(file, line)) { 
    
        lineSkip = !line.find("#");

        if (lineSkip) { 
            continue;
        }

        else if (!processStart && !instructionStart) { 
            numProcess = stoi(line);
            
            for (int pid = 0; pid < numProcess; pid++) { 
                Process process;
                Statistics stats;
                stats.pid = pid;

                for (int vpage = 0; vpage < NumVPages; vpage++){ 
                    PageTableEntry PTE;
                    process.PageTable.push_back(PTE);                
                }

                ProcessTable.push_back(process);
                StatTable.push_back(stats);
            } 

            processStart = true;
            VMAStart = true;           
        }

        else if (processStart && !instructionStart) { 
         
            if (VMAStart) { 
                numVMAs = stoi(line);
                VMAStart = false;
            }

            else if (!VMAStart) {
                istringstream iss(line);
                string numstr;
                VMA vma;
                vector<int> vma_vector;
            
                while(getline(iss, numstr, ' ')) { 
                    vma_vector.push_back(stoi(numstr));
                }

                vma.startVPage = vma_vector[0];
                vma.endVPage = vma_vector[1];
                vma.writeProtect = vma_vector[2];
                vma.fileMapped = vma_vector[3];

                ProcessTable[processNum].vmas.push_back(vma);
                VMANum++;

                if (VMANum == numVMAs) { 
                    VMANum = 0;
                    numVMAs = 0;
                    VMAStart = true;
                    processNum++;    

                    if (processNum == numProcess) { 
                        processStart = false;
                        VMAStart = false;
                        instructionStart = true;
                    }                          
                }
            }            
        }
          
        else if (!processStart && instructionStart) { 
            size_t spacePos = line.find(' ');
            char operation = line.substr(0, spacePos)[0]; 
            int vpage = stoi(line.substr(spacePos + 1)); 
            totalcost.currentInst = instructionNum;  
            
            if (operation == 'c') { 
                currentProcess = &ProcessTable[vpage];
                currentStats = &StatTable[vpage];
                currentPID = vpage;
                totalcost.contextSwitches++;
                totalcost.cost += 130;
                stringstream outputsCS;

                outputsCS << instructionNum
                          << ":" 
                          << " " 
                          << "==>" 
                          << " "
                          << operation
                          << " "
                          << currentPID
                          << endl;

                if (printOutputs) { 
                    printf("%s", outputsCS.str().c_str());
                }
            }

            else if (operation == 'r' || operation == 'w') {
                stringstream outputsRW;
                totalcost.cost++;

                outputsRW << instructionNum
                          << ":" 
                          << " " 
                          << "==>" 
                          << " "
                          << operation 
                          << " "
                          << vpage
                          << endl;    
            
                PageTableEntry* PTE = &currentProcess->PageTable[vpage];

                vector<VMA> vmas = currentProcess->vmas;                

                for (int i = 0; i < vmas.size(); i++) { 
                    if (vmas[i].startVPage <= vpage && vpage <= vmas[i].endVPage) { 
                        PTE->writeProtect = vmas[i].writeProtect;
                        PTE->fileMapped = vmas[i].fileMapped;
                        PTE->partofVMA = 1;
                        break;
                    }
                }
                    
                PTE->vpage = vpage;     
                PTE->referenced = 1; 

                if (printOutputs) { 
                    printf("%s", outputsRW.str().c_str());
                }

                mapPTE(vpage, currentPID, pager);

                if (operation == 'w') { 

                    if (!PTE->writeProtect) { 
                        PTE->modified = 1;
                    }

                    else if (PTE->writeProtect) { 
                        if (printOutputs) { 
                            printf(" SEGPROT\n");
                        }
                        currentStats->segprot++;
                    }
                }
            }
            
            else if (operation == 'e') { 
                int exitPID = vpage;

                stringstream outputsEX;
                
                if (printOutputs) { 
                    printf("%d: ==> %c %d\n", instructionNum, operation, exitPID);

                }
                
                printf("EXIT current process %d\n", exitPID);

                totalcost.processExits++;
                totalcost.cost += 1230;

                for (int p = 0; p < ProcessTable[exitPID].PageTable.size(); p++) { 
                    PageTableEntry* PTE = &ProcessTable[exitPID].PageTable[p];
                    if (PTE->present) {  
                        Frame* freeFrame = &FrameTable[PTE->frameNum];

                        StatTable[exitPID].unmaps++;    
                        
                        outputsEX << " "
                                  << "UNMAP"
                                  << " "
                                  << exitPID
                                  << ":"
                                  << PTE->vpage
                                  << endl;

                        if (PTE->modified) { 
                                
                            if (PTE->fileMapped) { 
                                outputsEX << " FOUT" << endl;
                                StatTable[exitPID].fouts++;
                            }
                            
                            PTE->modified = 0;
                        }                             
                                        
                        PTE->present = 0;    
                        freeFrame->allocationStatus = 0;
                        FreeFramePool.push_back(freeFrame);
                    }

                    if (PTE->out) { 
                        PTE->out = 0;
                    }                    
                }

                if (printOutputs) { 
                    printf("%s", outputsEX.str().c_str());
                }
            }
            
            instructionNum++;
        }
    }    
    
    file.close();

    if (printPT) { 

        for (int pid = 0; pid < ProcessTable.size(); pid++) {
            printf("PT[%d]: ", pid);
            Process process = ProcessTable[pid];

            for (int pt = 0; pt < process.PageTable.size(); pt++) {
                PageTableEntry PTE = process.PageTable[pt];

                if (!PTE.present) {

                    if (PTE.out) {
                        printf("#");

                        if (pt != process.PageTable.size() - 1) {
                            printf(" ");
                        }                    
                    } 

                    else if (!PTE.out) {
                        printf("*");

                        if (pt != process.PageTable.size() - 1) {
                            printf(" ");
                        }                    
                    }
                } 
                
                else if (PTE.present) {
                    string s = "";
                    
                    if (PTE.referenced) {
                        s = s + "R";
                    } 
                    
                    else if (!PTE.referenced) {
                        s = s + "-";
                    }
                    
                    if (PTE.modified) {
                        s = s + "M";
                    } 
                    
                    else if (!PTE.modified) {
                        s = s + "-";
                    }

                    if (PTE.out) {
                        s = s + "S";
                    } 
                    
                    else if (!PTE.out) {
                        s = s + "-";
                    }
                    
                    printf("%d:%s", pt, s.c_str());

                    if (pt != process.PageTable.size() - 1) {
                        printf(" ");
                    }
                }
            }
            printf("\n");
        }
    }
    
    if (printF) { 
        printf("FT: ");
        for (int f = 0; f < FrameTable.size(); f++) {
            Frame frame = FrameTable[f];

            if (!frame.allocationStatus) { 
                printf("*");
            }

            else if (frame.allocationStatus) { 
                printf("%d:%d", frame.pid, frame.vpage);
            }
            
            if (f != FrameTable.size() - 1) {
                printf(" ");
            }
        }

        printf("\n");
    }

    if (printSummary) { 
        for (int s = 0; s < StatTable.size(); s++) { 
            printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                    StatTable[s].pid,
                    StatTable[s].unmaps, 
                    StatTable[s].maps, 
                    StatTable[s].ins, 
                    StatTable[s].outs,
                    StatTable[s].fins, 
                    StatTable[s].fouts, 
                    StatTable[s].zeros,
                    StatTable[s].segv, 
                    StatTable[s].segprot);

            totalcost.cost += 410 * StatTable[s].unmaps + 
                            350 * StatTable[s].maps + 
                            3200 * StatTable[s].ins + 
                            2750 * StatTable[s].outs + 
                            2350 * StatTable[s].fins + 
                            2800 * StatTable[s].fouts + 
                            150  * StatTable[s].zeros + 
                            440 * StatTable[s].segv + 
                            410 * StatTable[s].segprot;     
        }

        printf("TOTALCOST %lu %lu %lu %llu %lu\n",
            totalcost.currentInst + 1, 
            totalcost.contextSwitches, 
            totalcost.processExits, 
            totalcost.cost, 
            totalcost.sizePTE);
    }

    // Attention: Find a way to implement 
    // this without an error.
    // for (auto& freeFrame : FreeFramePool) {
    //     delete freeFrame; 
    // }
    
    return 0;
}






