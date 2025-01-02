#pragma once

#include <vector>
#include <iostream>
#include <stdexcept>
#include <optional>
#include <map>
#include <set>

namespace Appledore
{
    // Tag structures
    struct DirectedG
    {
    };
    struct UndirectedG
    {
    };
    struct UnweightedG
    {
    };
    class GraphVertex
    {
    public:
        size_t id;

        static size_t nextId;

        GraphVertex()
        {
            id = nextId++;
        }
        bool operator<(const GraphVertex &other) const
        {
            return id < other.id;
        }
    };
    size_t Appledore::GraphVertex::nextId = 1;
    template <typename EdgeType>
    struct EdgeInfo
    {
        EdgeType value;

        EdgeInfo(const EdgeType &value) : value(value) {}
    };

    // GraphMatrix class template
    template <typename VertexType, typename EdgeType, typename Direction>
    class GraphMatrix
    {
    public:
        GraphMatrix()
            : isWeighted(!std::is_same_v<EdgeType, UnweightedG>),
              isDirected(std::is_same_v<Direction, DirectedG>) {}

        template <typename... Vertices>
        void addVertex(Vertices &&...vertices)
        {
            auto add_single_vertex = [this](const VertexType &vertex)
            {
                if (vertexToIndex.count(vertex))
                    return;
                size_t newIndex = numVertices++;
                vertexToIndex[vertex] = newIndex;
                indexToVertex.push_back(vertex);
            };

            (add_single_vertex(std::forward<Vertices>(vertices)), ...);

            adjacencyMatrix.resize(numVertices * numVertices, std::nullopt);
        }

        bool operator()(const VertexType src, const VertexType &dest)
        {
            if (!vertexToIndex.count(src) || !vertexToIndex.count(dest))
            {
                return false;
            }

            size_t srcIndex = vertexToIndex[src];
            size_t destIndex = vertexToIndex[dest];

            if (adjacencyMatrix[getIndex(srcIndex, destIndex)].has_value())
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        // for weighted.
        void addEdge(const VertexType &src, const VertexType &dest, const EdgeType &edge)
        {
            if (!vertexToIndex.count(src) || !vertexToIndex.count(dest))
                throw std::invalid_argument("One or both vertices do not exist");

            size_t srcIndex = vertexToIndex.at(src);
            size_t destIndex = vertexToIndex.at(dest);

            adjacencyMatrix[getIndex(srcIndex, destIndex)] = EdgeInfo<EdgeType>(edge);

            if constexpr (std::is_same_v<Direction, UndirectedG>)
            {
                adjacencyMatrix[getIndex(destIndex, srcIndex)] = EdgeInfo<EdgeType>(edge);
            }
        }

        // for unweighted
        void addEdge(const VertexType &src, const VertexType &dest)
        {
            if (!vertexToIndex.count(src) || !vertexToIndex.count(dest))
            {
                throw std::invalid_argument("One or both vertices do not exist");
            }

            size_t srcIndex = vertexToIndex.at(src);
            size_t destIndex = vertexToIndex.at(dest);

            adjacencyMatrix[getIndex(srcIndex, destIndex)] = EdgeInfo<EdgeType>();

            if constexpr (std::is_same_v<Direction, UndirectedG>)
            {
                adjacencyMatrix[getIndex(destIndex, srcIndex)] = EdgeInfo<EdgeType>();
            }
        }

        // Remove edge
        void removeEdge(const VertexType &src, const VertexType &dest)
        {
            if (!vertexToIndex.count(src) || !vertexToIndex.count(dest))
                throw std::invalid_argument("One or both vertices do not exist");

            size_t srcIndex = vertexToIndex.at(src);
            size_t destIndex = vertexToIndex.at(dest);

            adjacencyMatrix[getIndex(srcIndex, destIndex)] = std::nullopt;

            if (!isDirected)
            {
                adjacencyMatrix[getIndex(destIndex, srcIndex)] = std::nullopt;
            }
        }

        // Check if edge exists
        bool hasEdge(const VertexType &src, const VertexType &dest) const
        {
            if (!vertexToIndex.count(src) || !vertexToIndex.count(dest))
                throw std::invalid_argument("One or both vertices do not exist");

            size_t srcIndex = vertexToIndex.at(src);
            size_t destIndex = vertexToIndex.at(dest);

            return adjacencyMatrix[getIndex(srcIndex, destIndex)].has_value();
        }

        // Get vertices
        const std::vector<VertexType> &getVertices() const
        {
            return indexToVertex;
        }

        const EdgeType &getEdge(VertexType &src, VertexType &dest) const
        {
            if (!vertexToIndex.count(src) || !vertexToIndex.count(dest))
            {
                throw std::invalid_argument("One of both vertices do not exist!");
            }
            size_t srcIndex = vertexToIndex.at(src);
            size_t destIndex = vertexToIndex.at(dest);

            auto &egdeValue = adjacencyMatrix[getIndex(srcIndex, destIndex)];

            if (egdeValue.has_value())
            {
                return egdeValue.value().value;
            }
            else
            {
                throw std::runtime_error("No Edge exists between the given vertices.");
            }
        }

        // Get all edges
        std::vector<std::tuple<VertexType, VertexType, EdgeType>> getAllEdges() const
        {
            std::vector<std::tuple<VertexType, VertexType, EdgeType>> edges;

            for (size_t srcIndex = 0; srcIndex < numVertices; ++srcIndex)
            {
                for (size_t destIndex = 0; destIndex < numVertices; ++destIndex)
                {
                    const auto &edgeValue = adjacencyMatrix[getIndex(srcIndex, destIndex)];
                    if (edgeValue.has_value())
                    {
                        edges.emplace_back(
                            indexToVertex[srcIndex],
                            indexToVertex[destIndex],
                            edgeValue.value().value);
                    }
                }
            }
            return edges;
        }

        // Get neighbors for a vertex (using only a std::set for both weighted and unweighted graphs)
        auto getNeighbors(const VertexType& vertex) const
        {
            if (!vertexToIndex.count(vertex))
            {
                throw std::invalid_argument("Vertex does not exist in the graph");
            }

            size_t vertexIndex = vertexToIndex.at(vertex);

            // Using a set to store neighbors
            std::set<VertexType> neighbors;

            for (size_t destIndex = 0; destIndex < numVertices; ++destIndex)
            {
                if (adjacencyMatrix[getIndex(vertexIndex, destIndex)].has_value())
                {
                    neighbors.insert(indexToVertex[destIndex]);
                }

                // For undirected graphs, also check the reverse direction
                if constexpr (std::is_same_v<Direction, UndirectedG>)
                {
                    if (adjacencyMatrix[getIndex(destIndex, vertexIndex)].has_value())
                    {
                        neighbors.insert(indexToVertex[destIndex]);
                    }
                }
            }

            return neighbors;
        }

    private:
        std::map<VertexType, size_t> vertexToIndex;
        std::vector<VertexType> indexToVertex;
        std::vector<std::optional<EdgeInfo<EdgeType>>> adjacencyMatrix;
        size_t numVertices = 0;
        bool isDirected;
        bool isWeighted;
        inline size_t getIndex(size_t src, size_t dest) const
        {
            return src * numVertices + dest;
        }
    };
}
