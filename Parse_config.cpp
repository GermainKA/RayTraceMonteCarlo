#include <fstream>
#include <iostream>
#include <sstream>
#include "Config.h"

bool read_config(const std::string& filename, Config& cfg)
{
    std::ifstream in(filename);
    if(!in)
    {
        std::cerr << "Impossible d'open le fichier " << filename << std::endl;
        return false;
    }

    std::string line;
    while(std::getline(in, line))
    {
        if(line.empty() || line[0] == '#')
            continue;

        // SpliLine On Key/value
        std::istringstream iss(line);
        std::string key;
        int value; 
        if(!(iss >> key >> value)) 
        {
            continue;
        }

        if(key == "barycentriqueImg")       cfg.barycentriqueImg = (value != 0);
        else if(key == "noShadowsImg")      cfg.noShadowsImg     = (value != 0);
        else if(key == "fibonacciImg")      cfg.fibonacciImg     = (value != 0);
        else if(key == "montecarloconstpdfImg") cfg.montecarloconstpdfImg = (value != 0);
        else if(key == "montecarlodirectLiImg") cfg.montecarlodirectLiImg = (value != 0);

        else if(key == "withsky") cfg.withsky = (value != 0);
        else if(key == "bdrf")    cfg.bdrf    = (value != 0);
        else if(key == "N")       cfg.N       = value;
    }

    in.close();
    return true;
}