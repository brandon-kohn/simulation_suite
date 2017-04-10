//
//! Copyright � 2008-2011
//! Brandon Kohn
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <stk/geometry/geometry_kernel.hpp>
#include <stk/graph/temporary_vertex_graph_adaptor.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace stk;

namespace {
    enum class VertexType
    {
            Obstacle
        ,   Target
    };

    enum class EdgeType
    {
            Real
        ,   Virtual
    };

    struct VertexProperties
    {
        VertexProperties(const point2& p = point2(0 * units::si::meters, 0 * units::si::meters), bool isConcave = false, VertexType t = VertexType::Obstacle)
            : position(p)
            , isConcave(isConcave)
            , type(t)
        {}

        point2 position;
        bool isConcave{ false };
        VertexType type;
    };

    struct EdgeProperties
    {
        EdgeProperties(double weight = 0., EdgeType t = EdgeType::Virtual)
            : weight(weight)
            , type(t)
        {}

        double weight;
        EdgeType type;
    };

    using Graph = boost::adjacency_list
        <
        boost::vecS,
        boost::vecS,
        boost::directedS,
        VertexProperties,
        EdgeProperties
        >;

    using Vertex = Graph::vertex_descriptor;
    using Edge = Graph::edge_descriptor;
}

TEST(DynamicNavigationTestSuite, adjacency_iterator_iteration_over_base)
{
    using adj_t = temporary_vertex_graph_adaptor<Graph>::adjacency_iterator;

    Graph g;
    auto p1 = point2{ 0.* units::si::meters, 0.*units::si::meters };
    auto p2 = point2{ 0.* units::si::meters, 1.*units::si::meters };
    auto v1 = boost::add_vertex(VertexProperties(p1, true, VertexType::Target), g);
    auto v2 = boost::add_vertex(VertexProperties(p2, true, VertexType::Target), g);
    double weight = geometrix::point_point_distance(p1, p2).value();
    EdgeProperties props = { weight, EdgeType::Virtual };
    Edge e; bool b;
    boost::tie(e, b) = boost::add_edge(v1, v2, props, g);

    auto p3 = point2{ 3.* units::si::meters, 1.*units::si::meters };
    temporary_vertex_graph_adaptor<Graph> ag(g, VertexProperties(p3, true, VertexType::Target), { {0, props}, {1, props} });
    auto v3 = ag.getAdaptedVertex();

    adj_t ai, aend;
    boost::tie(ai, aend) = boost::adjacent_vertices(v1, ag);
    EXPECT_EQ(*ai++, 1);
    EXPECT_EQ(ai, aend);
}

TEST(DynamicNavigationTestSuite, adjacency_iterator_iteration_over_new)
{
    using adj_t = temporary_vertex_graph_adaptor<Graph>::adjacency_iterator;

    Graph g;
    auto p1 = point2{ 0.* units::si::meters, 0.*units::si::meters };
    auto p2 = point2{ 0.* units::si::meters, 1.*units::si::meters };
    auto v1 = boost::add_vertex(VertexProperties(p1, true, VertexType::Target), g);
    auto v2 = boost::add_vertex(VertexProperties(p2, true, VertexType::Target), g);
    double weight = geometrix::point_point_distance(p1, p2).value();
    EdgeProperties props = { weight, EdgeType::Virtual };
    Edge e; bool b;
    boost::tie(e, b) = boost::add_edge(v1, v2, props, g);


    auto p3 = point2{ 3.* units::si::meters, 1.*units::si::meters };
    temporary_vertex_graph_adaptor<Graph> ag(g, VertexProperties(p3, true, VertexType::Target), { { 0, props },{ 1, props } });
    auto v3 = ag.getAdaptedVertex();
    
    adj_t ai, aend;
    boost::tie(ai, aend) = boost::adjacent_vertices(v3, ag);
    EXPECT_EQ(*ai++, 0);
    EXPECT_EQ(*ai++, 1);
    EXPECT_EQ(ai, aend);
}

TEST(DynamicNavigationTestSuite, vertex_iterator_over_graph)
{
    using iterator = temporary_vertex_graph_adaptor<Graph>::vertex_iterator;

    Graph g;
    auto p1 = point2{ 0.* units::si::meters, 0.*units::si::meters };
    auto p2 = point2{ 0.* units::si::meters, 1.*units::si::meters };
    auto v1 = boost::add_vertex(VertexProperties(p1, true, VertexType::Target), g);
    auto v2 = boost::add_vertex(VertexProperties(p2, true, VertexType::Target), g);
    double weight = geometrix::point_point_distance(p1, p2).value();
    EdgeProperties props = { weight, EdgeType::Virtual };
    Edge e; bool b;
    boost::tie(e, b) = boost::add_edge(v1, v2, props, g);
    
    auto p3 = point2{ 3.* units::si::meters, 1.*units::si::meters };
    temporary_vertex_graph_adaptor<Graph> ag(g, VertexProperties(p3, true, VertexType::Target), { { 0, props },{ 1, props } });
    auto v3 = ag.getAdaptedVertex();

    iterator ai, aend;
    boost::tie(ai, aend) = boost::vertices(ag);
    EXPECT_EQ(*ai++, 0);
    EXPECT_EQ(*ai++, 1);
    EXPECT_EQ(*ai++, 2);
    EXPECT_EQ(ai, aend);
}

TEST(DynamicNavigationTestSuite, edge_iterator_over_graph)
{
    using iterator = temporary_vertex_graph_adaptor<Graph>::edge_iterator;

    Graph g;
    auto p1 = point2{ 0.* units::si::meters, 0.*units::si::meters };
    auto p2 = point2{ 0.* units::si::meters, 1.*units::si::meters };
    auto v1 = boost::add_vertex(VertexProperties(p1, true, VertexType::Target), g);
    auto v2 = boost::add_vertex(VertexProperties(p2, true, VertexType::Target), g);
    double weight = geometrix::point_point_distance(p1, p2).value();
    EdgeProperties props = { weight, EdgeType::Virtual };
    Edge e; bool b;
    boost::tie(e, b) = boost::add_edge(v1, v2, props, g);

    auto p3 = point2{ 3.* units::si::meters, 1.*units::si::meters };
    temporary_vertex_graph_adaptor<Graph> ag(g, VertexProperties(p3, true, VertexType::Target), { { v1, props },{ v2, props } });
    auto v3 = ag.getAdaptedVertex();

    iterator ai, aend;
    boost::tie(ai, aend) = boost::edges(ag);
    EXPECT_EQ(boost::source(*ai, ag), v1);
    EXPECT_EQ(boost::target(*ai, ag), v2);
    ++ai;
    EXPECT_EQ(boost::source(*ai, ag), v3);
    EXPECT_EQ(boost::target(*ai, ag), v1);
    ++ai;
    EXPECT_EQ(boost::source(*ai, ag), v3);
    EXPECT_EQ(boost::target(*ai, ag), v2);
    ++ai;
    EXPECT_EQ(ai, aend);
}

TEST(DynamicNavigationTestSuite, edge_properties)
{
    using iterator = temporary_vertex_graph_adaptor<Graph>::edge_iterator;

    Graph g;
    auto p1 = point2{ 0.* units::si::meters, 0.*units::si::meters };
    auto p2 = point2{ 0.* units::si::meters, 1.*units::si::meters };
    auto v1 = boost::add_vertex(VertexProperties(p1, true, VertexType::Target), g);
    auto v2 = boost::add_vertex(VertexProperties(p2, true, VertexType::Target), g);
    double weight = geometrix::point_point_distance(p1, p2).value();
    EdgeProperties props = { weight, EdgeType::Virtual };
    Edge e; bool b;
    boost::tie(e, b) = boost::add_edge(v1, v2, props, g);

    auto p3 = point2{ 3.* units::si::meters, 1.*units::si::meters };
    temporary_vertex_graph_adaptor<Graph> ag(g, VertexProperties(p3, true, VertexType::Target), { { v1, props },{ v2, props } });
    auto v3 = ag.getAdaptedVertex();

    auto w = boost::get(&EdgeProperties::weight, ag, e);
    EXPECT_EQ(weight, w);

    auto e31 = boost::edge(v3, v1, ag).first;

    w = boost::get(&EdgeProperties::weight, ag, e);
    EXPECT_EQ(weight, w);
}

TEST(DynamicNavigationTestSuite, vertex_properties)
{
    using iterator = temporary_vertex_graph_adaptor<Graph>::edge_iterator;

    Graph g;
    auto p1 = point2{ 0.* units::si::meters, 0.*units::si::meters };
    auto p2 = point2{ 0.* units::si::meters, 1.*units::si::meters };
    auto v1 = boost::add_vertex(VertexProperties(p1, true, VertexType::Target), g);
    auto v2 = boost::add_vertex(VertexProperties(p2, true, VertexType::Target), g);
    double weight = geometrix::point_point_distance(p1, p2).value();
    EdgeProperties props = { weight, EdgeType::Virtual };
    Edge e; bool b;
    boost::tie(e, b) = boost::add_edge(v1, v2, props, g);

    auto p3 = point2{ 3.* units::si::meters, 1.*units::si::meters };
    temporary_vertex_graph_adaptor<Graph> ag(g, VertexProperties(p3, true, VertexType::Obstacle), { { v1, props },{ v2, props } });
    auto v3 = ag.getAdaptedVertex();

    auto t = boost::get(&VertexProperties::type, ag, v1);
    EXPECT_EQ(VertexType::Target, t);
    
    t = boost::get(&VertexProperties::type, ag, v3);
    EXPECT_EQ(VertexType::Obstacle, t);
}
