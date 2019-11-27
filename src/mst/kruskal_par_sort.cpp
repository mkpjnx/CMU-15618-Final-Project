//
// Created by Xuren Zhou on 11/17/19.
//
#include <algorithm>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include "disjoint_set.h"
#include "graph.h"

#define PAR_SORT_THRESHOLD 8192


void parallelQuickSort(std::vector<Edge>::iterator begin, std::vector<Edge>::iterator end) {
    auto cmp = [](const Edge& lhs, const Edge& rhs) { return lhs.weight < rhs.weight; };
    size_t dist = std::distance(begin, end);
//    std::cout << dist << std::endl;
    if (dist <= PAR_SORT_THRESHOLD) {
        sort(begin, end, cmp);
        return;
    }

    auto pivot = *std::next(begin, dist / 2); // FIXME: random chosen
    std::vector<Edge>::iterator iter[4];
    iter[0] = begin;
    iter[3] = end;

    iter[1] = std::partition(begin, end, [pivot](const Edge& edge) {return edge.weight < pivot.weight; });
    iter[2] = std::partition(iter[1], end, [pivot](const Edge &edge) {
        return pivot.weight >= edge.weight;
    });

    for (int i = 0; i < 2; i++) {
#pragma omp task
        parallelQuickSort(iter[2 * i], iter[2 * i + 1]);
    }
#pragma omp taskwait
}

Graph kruskalMSTSequential(const Graph &graph) {
    struct timeval start, end;
    double duration;
    double total = 0;

    // Copy and sort the edges by weight.
    std::vector<Edge> edges = graph.edges;
    gettimeofday(&start, nullptr);

#pragma omp parallel
#pragma omp single nowait
    parallelQuickSort(edges.begin(), edges.end());

    gettimeofday(&end, nullptr);
    duration = (double)((end.tv_sec  - start.tv_sec) * 1000000u +
                        end.tv_usec - start.tv_usec) / 1.e6;
    total += duration;
    std::cout << "Sort time: " << duration << " s." << std::endl;

    // Kruskal algorithm.
    gettimeofday(&start, nullptr);
    DisjointSet ds(graph.num_vertices);
    std::vector<Edge> mst_edges;
    for (const Edge e : edges) {
        if (!ds.belongSameSet(e.from, e.to)) {
            ds.unionSet(e.from, e.to);
            mst_edges.push_back(e);
        }
    }
    gettimeofday(&end, nullptr);
    duration = (double)((end.tv_sec  - start.tv_sec) * 1000000u +
                        end.tv_usec - start.tv_usec) / 1.e6;
    std::cout << "Merging time: " << duration << " s." << std::endl;

    total += duration;
    std::cout << "total time: " << total << " s." << std::endl;
    std::cout << "sort percent: " << 1 - duration / total << std::endl;
    return {graph.num_vertices, mst_edges};
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Invalid command: bin/kruskal_par_sort <data_path>"
                  << std::endl;
        return 1;
    }
    std::string filename = argv[1];
    std::cout << "File name: " << filename << std::endl;
    struct timeval start, end;
    double duration;

    gettimeofday(&start, nullptr);
    Graph graph;
    graph.loadGraph(filename);
    gettimeofday(&end, nullptr);
    duration = (double)((end.tv_sec  - start.tv_sec) * 1000000u +
                        end.tv_usec - start.tv_usec) / 1.e6;
    std::cout << "loading time: " << duration << " s." << std::endl;

    // Execute the algorithm and print the MST.
    Graph mst = kruskalMSTSequential(graph);

    // Save the result.
    std::string output_filename = filename + ".par_sort.output";
    std::cout << "Saving MST to " << output_filename << "." << std::endl;
    mst.saveGraph(output_filename);

    return 0;
}
