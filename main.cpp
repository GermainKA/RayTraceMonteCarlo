//! \file tuto_rayons.cpp

#include <vector>
#include <cfloat>
#include <chrono>

#include "vec.h"
#include "mat.h"
#include "color.h"
#include "image.h"
#include "image_io.h"
#include "image_hdr.h"
#include "orbiter.h"
#include "wavefront.h"
#include "Scene.h"

#include "Config.h"

int main(const int argc, const char **argv)
{
    Config cfg;

    bool ok = read_config("TP/TP3/RayTraceConfig.txt", cfg);
    if (!ok)
    {
        std::cerr << "Erreur lecture fichiers de configuration" << std::endl;
        std::cerr << "Config par defaut" << std::endl;
    }

    const char *mesh_filename = "data/cornell.obj";
    if (argc > 1)
        mesh_filename = argv[1];

    const char *orbiter_filename = "data/cornell_orbiter.txt";
    if (argc > 2)
        orbiter_filename = argv[2];

    Orbiter camera;
    if (camera.read_orbiter(orbiter_filename) < 0)
        return 1;

    Mesh mesh = read_mesh(mesh_filename);
    Scene *m_Scene = new Scene(mesh);

    //
    Image image(1024, 768) ;

    // recupere les transformations pour generer les rayons
    camera.projection(image.width(), image.height(), 45);
    Transform model = Identity();
    Transform view = camera.view();
    Transform projection = camera.projection();
    Transform viewport = camera.viewport();
    Transform inv = Inverse(viewport * projection * view * model);

    auto start = std::chrono::high_resolution_clock::now();

    // parcours tous les pixels de l'image
    #pragma omp parallel for schedule(dynamic, 1)
    for (int y = 0; y < image.height(); y++)
    {
        std::random_device hwseed;
        Sampler rng(hwseed());

        for (int x = 0; x < image.width(); x++)
        {
            // generer le rayon au centre du pixel
            Point origine = inv(Point(x + float(0.5), y + float(0.5), 0));
            Point extremite = inv(Point(x + float(0.5), y + float(0.5), 1));
            Ray ray(origine, extremite);

            // calculer les intersections avec tous les triangles
            float tmax = ray.tmax; // extremite du rayon
            Hit hit = m_Scene->closestHit(ray, tmax);

            if (hit)
            {

                Point p = ray.o + hit.t * ray.d;
                Color color = Black();
                if (cfg.barycentriqueImg)
                {
                    image(x, y) = Color(1 - hit.u - hit.v, hit.u, hit.v);
                }
                else if (cfg.noShadowsImg)
                {
                    m_Scene->withoutShadow(image(x, y),hit, cfg.bdrf);
                }
                else if (cfg.fibonacciImg)
                {
                    m_Scene->fibonacciSampling(image(x, y),p, hit,cfg.withsky, cfg.bdrf, cfg.N);

                }
                else if (cfg.montecarloconstpdfImg)
                {
                    m_Scene->montCarloConstPdf(image(x, y),p, hit, rng, cfg.withsky, cfg.bdrf, cfg.N);
                   
                }
                else if (cfg.montecarlodirectLiImg)
                {

                    m_Scene->montCarloAreaPdf(image(x, y),p, hit, rng, cfg.bdrf, cfg.N);

                }
                else
                {
                    color = Black();
                }

            }
        }
    }

    auto stop = std::chrono::high_resolution_clock::now();
    int cpu = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    printf("%dms\n", cpu);

    write_image(image, "render.png");
    write_image_hdr(image, "render.hdr");

    delete m_Scene;
    return 0;
}
