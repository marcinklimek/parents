#include "mainwindow.h"
#include <QApplication>
#include <cstdint>
#include <vector>
#include <random>
#include <numeric>
#include <functional>
#include <iostream>
#include <fstream>
#include <limits>

const size_t max_num_wisdom     =   10;
const size_t max_species        =  5000;
const size_t max_species_cross  =   20;

class Random
{
    std::random_device r;
    std::mt19937 e;
    std::normal_distribution<> normal_dist;
    std::uniform_int_distribution<> uniform_dist;

public:

    Random() : uniform_dist(0, max_species-1)
    {
        std::default_random_engine re(r());

        uniform_dist(re);

        std::uniform_int<uint64_t> uniform_dist_start(0, 10); // for initial purposes
        int mean = uniform_dist_start(re);
        std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
        e = std::mt19937(seed);

        normal_dist = std::normal_distribution<>(mean, 2);
    }

    uint64_t get()
    {
        return normal_dist(e);
    }

    size_t get_uniform()
    {
        return uniform_dist(e);
    }

};


Random random;

class Species
{
    std::vector<uint64_t> _wisdom;
    uint64_t _score;

public:

    Species() :  _score(0)
    {
    }

    void init()
    {
       for(size_t i=0; i < max_num_wisdom; ++i)
           _wisdom.emplace_back(random.get_uniform());
    }

    void add_wisdom(uint64_t w)
    {
        _wisdom.emplace_back(w);
    }

    uint64_t operator()(size_t idx)
    {
        return _wisdom[idx];
    }

    uint64_t score()
    {
        if (_score == 0)
            _score = std::accumulate(_wisdom.begin(), _wisdom.end(), 0);

        return _score;
    }

    uint64_t score_diff(Species s2)
    {
        return (score() > s2.score()) ? (score() - s2.score()): (s2.score() - score());
    }

    Species cross(Species& partner)
    {
        Species child;

        for(size_t idx=0; idx<_wisdom.size(); idx++)
        {
            size_t r = random.get() % 10;

            if ( r > 8 )
                child.add_wisdom( _wisdom[idx] + partner(idx) );
            else if ( r > 5 )
                child.add_wisdom( _wisdom[idx] );
            else
                child.add_wisdom( partner(idx) );
        }

        return child;
    }
};

std::vector<Species> population;
std::vector<Species> new_population;

static bool compare_score(Species& s1, Species& s2)
{
    return s1.score() < s2.score();
}

static bool compare_score2(Species& s1, Species& s2)
{
    return s1.score() > s2.score();
}

bool generation()
{
    new_population.clear();

    auto result = std::max_element(population.begin(), population.end(), compare_score);
    uint64_t max_score = result->score();

    if (max_score > 18446744071749127043LL)
        return false;

    // new child
    for(int x = 0; x < max_species_cross; ++x)
    {
        uint64_t score_diff = std::numeric_limits<uint64_t>::max();
        size_t s1, s2;

        do
        {
            s1 = random.get_uniform();
            s2 = random.get_uniform();

            while (s1 == s2)
                s2 = random.get_uniform();

            score_diff = population[s1].score_diff(population[s2]);

        } while (score_diff > (max_score>>1));

        new_population.emplace_back( population[s1].cross(population[s2]) );
    }

    // kill some
    for(int x = 0; x < max_species_cross; ++x)
    {

        size_t s1 = random.get_uniform();

        while (s1 >= population.size())
            s1 = random.get_uniform();

        population.erase( population.begin() + s1);
    }

    for (auto& spec: population)
        new_population.emplace_back( spec );

    population.clear();

    foreach (auto s, new_population)
        population.emplace_back(s);

    std::sort (population.begin(), population.end(), compare_score2);

    return true;
}


void dump()
{
    std::ofstream outfile ("parents.csv", std::fstream::out | std::fstream::app);

    std::vector<uint64_t> list;

    for(size_t i=0; i<max_species; i+=900)
        list.push_back( (*(population.begin()+i)).score());

    foreach (auto s, list)
    {
        outfile << s << ", ";
    }

    outfile << std::endl;
}

void init()
{
    population.clear();

    for(int x = 0; x < max_species; ++x)
    {
        auto s = Species();
        s.init();
        population.emplace_back( s );
    }
}

int main(int argc, char *argv[])
{
    //QApplication a(argc, argv);
    //MainWindow w;
    //w.show();
    //return a.exec();


    init();
    bool run = true;
    uint i = 0;
    while(run)
    {
        std::cout << "Generation #" <<i << std::endl;

        if ( i % 200 == 0)
            dump();

        run = generation();
        i++;
    }

    dump();

    return 0;
}
