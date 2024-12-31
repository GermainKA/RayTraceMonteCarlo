#pragma once
#include <string>

struct Config
{
    bool barycentriqueImg ;
    bool noShadowsImg ;
    bool fibonacciImg ;
    bool montecarloconstpdfImg ;
    bool montecarlodirectLiImg ;

    bool withsky ;
    bool bdrf;
    int N;
    Config(){
        noShadowsImg = false ;
        barycentriqueImg = false;
        fibonacciImg = false;
        montecarloconstpdfImg = false;
        montecarlodirectLiImg = true;

        bdrf = true;
        withsky = false;
        N = 64 ;
    }
};
bool read_config(const std::string& filename, Config& cfg);