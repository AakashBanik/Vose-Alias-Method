#include <iostream>
#include <vector>
#include <numeric>
#include <string>
#include <functional>
#include <algorithm>
#include <random>
#include <chrono>
#include <deque>

class AliasVecMethods{

private:
    std::vector<double> weights;
    int dim;
    int sum;
    std::vector<double> probs, initial_probs, alias;
    std::deque<int> large, small;
    int small_count = 0, large_count = 0;

public:
    AliasVecMethods() = default;

    explicit AliasVecMethods(std::vector<double> &weights):
    weights(weights),
    dim(weights.size()),
    sum(std::accumulate(weights.begin(), weights.end(), 0.0))
    {
        probs.reserve(dim);
        initial_probs.reserve(dim);
        alias.reserve(dim);
    }

    AliasVecMethods* init(){
        for (int i = 0; i != dim; i++){
            initial_probs.emplace_back(weights[i] * dim / sum);
        }

        int i = 0;
        for (auto &x : initial_probs)
        {
            if(x < 1)
                small.emplace_back(i);
            else
                large.emplace_back(i);

            i++;
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

    void populate_tables(){

        int l=0, g=0;

        while(small.size() != 0 && large.size() != 0){
            l = small.front(); //element exists ar n-1 not n (n is not valid for 0 index)
            g = large.front();

            small.pop_front();
            large.pop_front();

            probs[l] = initial_probs[l];
            alias[l] = g;

            initial_probs[g] = (initial_probs[g] + initial_probs[l]) - 1;

            if(initial_probs[g] < 1)
                small.emplace_back(g);
            else
                large.emplace_back(g);

        }

        while(large.size() > 0){
            probs[large.front()] = 1;
            large.pop_front();
        }
        while(small.size() > 0){
            probs[small.front()] = 1;
            small.pop_front();
        }
    }

    auto decision(){

        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_real_distribution<double> dist(0,dim);
        auto side = dist(mt);
        std::bernoulli_distribution coin_flip{probs[side]};
        bool value = coin_flip(mt);

        if(value) //if heads then
            return static_cast<int>(side); //call a side
        else
            return static_cast<int>(alias[side]);
    }

};