#pragma once
#include "Function.h"



class Scene
{
    private:
        Mesh m_Mesh_;
        std::vector<Triangle> m_Triangles_;
        std::vector<Source> m_Sources_;

    public:
        Scene(const Mesh& mesh);
        ~Scene();
        Hit occluded(const Point &p,const Vector& n, const Vector& d);
        Hit closestOccluded(const Point &p, const Vector &n, const Vector &d);
        Hit intersect(const Ray &ray, const float tmax);
        Hit closestHit(const Ray &ray, float& tmax);
        bool visible(const Point& p,const Point& q );
        void withoutShadow(Color& color,const Hit& hit,bool bdrf = true);
        void fibonacciSampling(Color& color,const Point& p,const Hit& hit,bool withsky,bool bdrf = true,int N = 64) ;
        void montCarloConstPdf(Color& color,const Point &p, const Hit &hit,Sampler& rng, bool withsky,bool bdrf, int N);
        void montCarloAreaPdf(Color& color,const Point &p, const Hit &hit,Sampler& rng, bool bdrf, int N);

        int m_NbrTriangles ;
        static const int K = 32;
        static const int I = 2; //Intensité d'une l'emission du ciel


};


