
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010 Francois Beaune
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef APPLESEED_FOUNDATION_MATH_KNN_KNN_QUERY_H
#define APPLESEED_FOUNDATION_MATH_KNN_KNN_QUERY_H

// appleseed.foundation headers.
#include "foundation/core/concepts/noncopyable.h"
#include "foundation/math/knn/knn_answer.h"
#include "foundation/math/knn/knn_tree.h"
#include "foundation/math/distance.h"
#include "foundation/math/vector.h"
#include "foundation/platform/compiler.h"

// Standard headers.
#include <cassert>
#include <cstddef>

namespace foundation {
namespace knn {

template <typename T, size_t N>
class Query
  : public NonCopyable
{
  public:
    typedef T ValueType;
    static const size_t Dimension = N;

    typedef Vector<T, N> VectorType;
    typedef Tree<T, N> TreeType;
    typedef Answer<T> AnswerType;

    Query(
        const TreeType&     tree,
        AnswerType&         answer);

    void run(const VectorType& query_point);

  private:
    typedef typename TreeType::NodeType NodeType;

    const TreeType&         m_tree;
    AnswerType&             m_answer;
};

typedef Query<float, 2>  Query2f;
typedef Query<double, 2> Query2d;
typedef Query<float, 3>  Query3f;
typedef Query<double, 3> Query3d;


//
// Implementation.
//

template <typename T, size_t N>
inline Query<T, N>::Query(
    const TreeType&         tree,
    AnswerType&             answer)
  : m_tree(tree)
  , m_answer(answer)
{
}

template <typename T, size_t N>
inline void Query<T, N>::run(const VectorType& query_point)
{
    assert(!m_tree.empty());

    m_answer.clear();

    const VectorType* RESTRICT points = &m_tree.m_points.front();
    const size_t* RESTRICT indices = &m_tree.m_indices.front();
    const NodeType* RESTRICT nodes = &m_tree.m_nodes.front();

    //
    // 1. Find the leaf node containing the query point.
    //

    const NodeType* RESTRICT parent_node = nodes;

    while (parent_node->is_interior())
    {
        const size_t split_dim = parent_node->get_split_dim();
        const ValueType split_abs = parent_node->get_split_abs();

        parent_node = nodes + parent_node->get_child_node_index();

        if (query_point[split_dim] >= split_abs)
            ++parent_node;
    }

    //
    // 2. Collect the points from this leaf node, and compute an upper bound on distance.
    //

    ValueType max_distance(0.0);

    const size_t* RESTRICT index_ptr = indices + parent_node->get_point_index();
    const size_t* RESTRICT index_end = index_ptr + parent_node->get_point_count();

    while (index_ptr < index_end)
    {
        // Fetch the point and compute its distance to the query point.
        const size_t point_index = *index_ptr++;
        const VectorType& point = points[point_index];
        const ValueType distance = square_distance(point, query_point);

        // Add this point to the answer.
        m_answer.insert(point_index, distance);

        // Update the upper bound on distance.
        if (max_distance < distance)
            max_distance = distance;
    }

    //
    // 3. Traverse again the tree and update the set of neighbors.
    //

    const size_t StackSize = 64;
    const NodeType* stack[StackSize];
    const NodeType** stack_ptr = stack;

    const NodeType* RESTRICT node = nodes;

    while (true)
    {
        // Traverse the tree until a leaf is reached.
        while (node->is_interior())
        {
            const size_t split_dim = node->get_split_dim();
            const ValueType split_abs = node->get_split_abs();
            const ValueType query_abs = query_point[split_dim];
            const ValueType distance = square(split_abs - query_abs);

            node = nodes + node->get_child_node_index();

            if (query_abs < split_abs)
            {
                // Follow the left node.
                if (distance < max_distance)
                    *stack_ptr++ = node + 1;
            }
            else
            {
                // Follow the right node.
                if (distance < max_distance)
                    *stack_ptr++ = node;
                ++node;
            }
        }

        if (node != parent_node)
        {
            const size_t* RESTRICT index_ptr = indices + node->get_point_index();
            const size_t* RESTRICT index_end = index_ptr + node->get_point_count();

            while (index_ptr < index_end)
            {
                // Fetch the point and compute its distance to the query point.
                const size_t point_index = *index_ptr++;
                const VectorType& point = points[point_index];
                const ValueType distance = square_distance(point, query_point);

                // Insert this point to the answer.
                m_answer.insert(point_index, distance);

                // Update the upper bound on distance.
                max_distance = m_answer.farthest().m_distance;
            }
        }

        // Terminate traversal if there is no more nodes to visit.
        if (stack_ptr == stack)
            break;

        // Pop the next node from the stack.
        node = *--stack_ptr;
    }

    //
    // 4. Transform the heap into a sorted array.
    //

    m_answer.sort();
}

}       // namespace knn
}       // namespace foundation

#endif  // !APPLESEED_FOUNDATION_MATH_KNN_KNN_QUERY_H
