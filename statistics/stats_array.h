#ifndef _STATS_ARR_H_
#define _STATS_ARR_H_

enum StatsArrType
{
    ArrInsert,
    ArrIncr
};
class StatsArr
{
public:
    void init(uint64_t size, StatsArrType type);
    void free();
    void clear();
    void quicksort(int low_idx, int high_idx);
    void resize();
    void insert(uint64_t item);
    void append(StatsArr arr);
    void print(FILE *f);
    void print(FILE *f, uint64_t min, uint64_t max);
    uint64_t get_idx(uint64_t idx);
    uint64_t get_percentile(uint64_t ile);
    uint64_t get_avg();

    uint64_t *arr;
    uint64_t size;
    uint64_t cnt;
    StatsArrType type;

private:
    pthread_mutex_t mtx;
};

#endif
