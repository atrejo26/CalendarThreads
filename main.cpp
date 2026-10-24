#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <unistd.h>
#include <pthread.h> //for threads and semaphores
#include <semaphore.h>

using namespace std;

void* runner(void*);

sem_t* runnersSync;
const int NUM_MONTHS = 12;

struct RunnerInfo {
    int myNumber;
    int perRow;
    int startDay;
    int numDays;
    int year;
    string name;
};

int main(int argc, char* argv[])
{
    if(argc != 4) {
        cout << "Usage: ./calThreads <monthsPerRow> <startDay 0-6> <year>\n";
        exit(0);
    }

    int perRow = atoi(argv[1]);   
    int janStart = atoi(argv[2]);   
    int year = atoi(argv[3]);

    if(perRow < 1 || janStart < 0 || janStart > 6) {
        cout<<"Invalid arguments"<<endl;
        exit(0);
    }

    string names[NUM_MONTHS] = {"January","February","March","April","May","June",
                                "July","August","September","October","November","December"};
    int daysIn[NUM_MONTHS] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
        daysIn[1] = 29;
    }

    runnersSync = new sem_t[NUM_MONTHS];
    sem_init(&runnersSync[0], 0, 1);
    for(int i = 1;i < NUM_MONTHS;i++) {
        sem_init(&runnersSync[i], 0, 0);
    }

    pthread_t* p_runners = new pthread_t[NUM_MONTHS];

    int start = janStart;
    for(int i = 0;i < NUM_MONTHS;i++) {
        RunnerInfo* runnerInfo = new RunnerInfo;
        runnerInfo->myNumber = i;
        runnerInfo->perRow = perRow;
        runnerInfo->startDay = start;
        runnerInfo->numDays = daysIn[i];
        runnerInfo->year = year;
        runnerInfo->name = names[i];
        start = (start + daysIn[i]) % 7;

        pthread_create(&p_runners[i], 0, runner, runnerInfo);
    }

    //wait for threads to complete
    for(int i = 0; i < NUM_MONTHS; i++) {
        pthread_join(p_runners[i], nullptr);
    }

    for(int i = 0; i < NUM_MONTHS; i++) {
        sem_destroy(&runnersSync[i]);
    }

    delete[] runnersSync;
    delete[] p_runners;

    return 0;
}
//--
void* runner(void* p_data) {
    RunnerInfo* p_runnerInfo = (RunnerInfo*)p_data;
    int myNum  = p_runnerInfo->myNumber;
    int perRow = p_runnerInfo->perRow;

    int rowStart = (myNum / perRow) * perRow;
    int rowEnd   = min(rowStart + perRow, NUM_MONTHS) - 1;

    for(int line = 0;line < 9;line++) {
        sem_wait(&runnersSync[myNum]);

        if(line == 0) {
            string title = p_runnerInfo->name + " " + to_string(p_runnerInfo->year);
            int len  = title.length();
            int left = (21 - len) / 2;
            cout<<string(left, ' ')<<title<<string(23 - left - len, ' ');
        }
        else if(line == 1) {
            const char days[] = "SMTWRFS";
            for(int c = 0; c < 7; c++)
                cout<<setw(3)<<days[c];
            cout<<"  ";
        }
        else if(line <= 7) {
            int week = line - 2;
            for(int col = 0; col < 7; col++)
            {
                int day = week * 7 + col - p_runnerInfo->startDay + 1;
                if(day < 1 || day > p_runnerInfo->numDays)
                    cout<<setw(3)<<"";
                else
                    cout<<setw(3)<<day;
            }
            cout<<"  ";
        }
        else {
            cout<<string(23, ' ');
        }

        if(myNum != rowEnd) {
            sem_post(&runnersSync[myNum + 1]);
        }
        else {
            cout<<endl;
            if(line < 8)
                sem_post(&runnersSync[rowStart]);
            else if(rowEnd + 1 < NUM_MONTHS)
                sem_post(&runnersSync[rowEnd + 1]);
        }
    }

    delete p_runnerInfo;
    return 0;
}