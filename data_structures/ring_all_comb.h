#ifndef _RING_ALL_COMB_H_
#define _RING_ALL_COMB_H_


#include <stdio.h>  /* printf, scanf, puts, NULL */
#include <stdlib.h> /* srand, rand */
#include <time.h>   /* time */
#include <iostream>
#include <vector>

class AllComb
{
private:
    uint64_t number_of_invloved_shards;
    uint64_t gnode_id;
    uint64_t shards_count;

public:
    std::vector<std::vector<uint64_t>> output;
    AllComb(uint64_t inv, uint64_t gnode, uint64_t sh_cnt) : number_of_invloved_shards(inv), gnode_id(gnode), shards_count(sh_cnt) {}

    std::vector<std::vector<uint64_t>> combine()
    {
        std::vector<uint64_t> curr;
        back_track(0, curr);
        return output;
    }

    void back_track(uint64_t start, std::vector<uint64_t> &curr)
    {
        if (curr.size() == number_of_invloved_shards)
        {
            std::vector<uint64_t> temp = curr;
            output.push_back(temp);
            return;
        }
        for (uint64_t i = start; i < shards_count; i++)
        {
            if (i == gnode_id)
                continue;
            curr.push_back(i);
            back_track(start + 1, curr);
            curr.pop_back();
        }
    }
};

#endif