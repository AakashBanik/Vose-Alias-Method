#include <iostream>
#include <vector>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <random>
#include <chrono>
#include "vec_imp.h"

#define sims 10000

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

template<typename T> class AliasMethods{
private:

    std::vector<T> weights;
    const int dim;
    double sum = 0;

    std::unique_ptr<double[]> probs, initial_probs,  alias;
    std::unique_ptr<int[]> large;
    std::unique_ptr<int[]> small;
    int small_count = 0, large_count = 0;


public:
    AliasMethods() = default;

    explicit AliasMethods(std::vector<T> &weights):
    weights(weights),
    dim(weights.size()),
    sum(std::accumulate(weights.begin(), weights.end(), 0.0))
    {
        probs = std::make_unique<double[]>(dim);
        initial_probs = std::make_unique<double[]>(dim);
        alias = std::make_unique<double[]>(dim);
        large = std::make_unique<int[]>(dim);
        small = std::make_unique<int[]>(dim);

        if(probs == nullptr || initial_probs == nullptr || alias == nullptr) {
            std::cout << "Mem Alloc Failure";
            exit(1);
        }

    }

    AliasMethods* init(){
        for (int i=0; i != dim; i++) {
            initial_probs[i] = static_cast<double>(weights[i]) * dim / sum;
        }

        for (int k = 0; k != dim; k++) {
            if (initial_probs[k]<1)
                small[small_count++] = k;
            else
                large[large_count++] = k;
        }

        return this;
    }

    void print(){
        int i=0;
        while(i < dim){
            std::cout<< probs[i] << std::endl;
            i++;
        }
    }

    /*
        1. Create arrays Alias and Prob, each of size n.
        2. Create two worklists, Small and Large.
        3. Multiply each probability by n.
        4. For each scaled probability pi:
        5. If pi<1, add i to Small.
        6. Otherwise (pi≥1), add i to Large.
        7. While Small and Large are not empty: (Large might be emptied first)
        8. Remove the first element from Small; call it l.
        9. Remove the first element from Large; call it g.
        10. Set Prob[l]=pl.
        11. Set Alias[l]=g.
        12. Set pg:=(pg+pl)−1. (This is a more numerically stable option.)
        13. If pg<1, add g to Small.
        14. Otherwise (pg≥1), add g to Large.
        15. While Large is not empty:
        16. Remove the first element from Large; call it g.
        17. Set Prob[g]=1.
        18. While Small is not empty: This is only possible due to numerical instability.
        19. Remove the first element from Small; call it l.
        20. Set Prob[l]=1.
     */
    void populate_tables(){

        int l=0, g=0;

        while(small_count && large_count){
            l = small[--small_count]; //element exists ar n-1 not n (n is not valid for 0 index)
            g = large[--large_count];
            probs[l] = initial_probs[l];
            alias[l] = g;

            initial_probs[g] = (initial_probs[g] + initial_probs[l]) - 1;

            if(initial_probs[g] < 1)
                small[small_count++] = g;
            else
                large[large_count++] = g;

        }

        while(large_count > 0)
            probs[large[--large_count]] = 1;
        while(small_count > 0)
            probs[small[--small_count]] = 1;
    }

    //roll a "dim" sided die to choose a side i
    //then roll a biased coin with probability of landing heads = probs[i] if true return i or return alias[i]
    auto decision(){
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<int> dist(0,dim-1);
        auto side = dist(mt);
        std::bernoulli_distribution coin_flip{probs[side]};
        bool value = coin_flip(mt);

        if(value) //if heads then
            return (int)side; //call a side
        else
            return static_cast<int>(alias[side]);
    }
};

int main(){

    std::vector<double> weights = {0.6, 0.2, 0.1, 0.05, 0.05};
    AliasMethods<double> *am = new AliasMethods(weights);
    //std::unique_ptr<double[]> p_values(new double[weights.size()]);
    //AliasVecMethods *am = new AliasVecMethods(weights);

    std::vector<int> val;
    val.reserve(sims);

    auto t1 = high_resolution_clock::now();
    am->init()->populate_tables();
    //avm.init();
    //avm.populate_tables();
    double rem = 0.0;
    double stddev, z_score;
    for (int i = 0; i < sims; i++)
    {
        rem = (double)i / sims;
        val.emplace_back(am->decision());

        double sum = std::accumulate(val.begin(), val.end(), 0.0);
        double m =  sum / val.size();
        
        double accum = 0.0;
        std::for_each (val.begin(), val.end(), [&](const double d) {
            accum += (d - m) * (d - m);
        });

        stddev = sqrt(accum / (val.size()));
        z_score = (val.back() - m) / stddev;

        auto t2 = (1.0 / rem) * ((double) duration_cast<std::chrono::nanoseconds>(high_resolution_clock::now() - t1).count() / 1e9);
        auto r = t2 - ((double)duration_cast<std::chrono::nanoseconds>(high_resolution_clock::now() - t1).count()/1e9);

        printf("%2.1f%% complete, Estimated %f seconds remaining, with StdDev: %f , and Z-Score: %f \r", (rem * 100.0), r, stddev, z_score);
    }
    auto t3 = high_resolution_clock::now();
    auto total = (double) duration_cast<std::chrono::nanoseconds >(t3-t1).count() / 1e9;


    int find_probs = 0;

    int sum = 0;
    for(int i=0; i< val.size(); i++){
        if(val[i] == find_probs)
            sum += 1;
    }
    //am->print();
    std::cout << "\n";
    std::cout << "Total Time (secs): " << total;
    std::cout << "\n";
    std::cout << "Actual Prob : " << weights[find_probs] / (double)std::accumulate(weights.begin(), weights.end(), 0.0) * 100
              << " \nVisualised Prob: " << (double)sum / sims * 100;
//    << "\nZ-Score: " << z_score << "\nStd Dev: " << stddev << "\n";

    return 0;
}