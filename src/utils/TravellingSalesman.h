//Copyright (c) 2015 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef TRAVELLINGSALESMAN_H
#define	TRAVELLINGSALESMAN_H

#include "intpoint.h" //For the Point type.

#include <algorithm> //For std::shuffle to randomly shuffle the array.
#include <list> //For std::list, which is used to insert stuff in the result in constant time.
#include <random> //For the RNG. Note: CuraEngine should be deterministic! Always use a FIXED SEED!

namespace cura
{

/*!
 * \brief Struct that holds all information of one element of the path.
 * 
 * It needs to know the actual element in the path, but also where the element's
 * own path starts and ends.
 * 
 * \tparam E The type of element data stored in this waypoint. Note that these
 * are copied into the waypoint on construction and out of the waypoint right
 * before deletion.
 */
template<class E> struct Waypoint
{
    /*!
     * \brief Constructs a new waypoint with the specified possible start and
     * end points and the specified element.
     * 
     * \param orientations The possible start and end points of the waypoints
     * for each orientation the element could be placed in.
     * \param element The element that's to be bound to this waypoint.
     */
    Waypoint(std::vector<std::pair<Point, Point>> orientations, E element) : orientation_indices(orientations), element(element)
    {
    }

    /*!
     * \brief The possible orientations in which the waypoint could be placed in
     * the path.
     * 
     * This defines in what direction or way the element in this waypoint should
     * be traversed in the final path. The Travelling Salesman solution only
     * requires the start and end point of this traversal in order to piece the
     * waypoint into the path.
     */
    std::vector<std::pair<Point, Point>> orientation_indices;

    /*!
     * \brief The actual element this waypoint holds.
     */
    E element;

    /*!
     * \brief The optimal orientation of this waypoint in the final path.
     * 
     * This is computed during the <em>TravellingSalesman::findPath</em>
     * function. It indicates an index in \link orientations that provides the
     * shortest path.
     */
    size_t best_orientation_index;
};

/*!
 * \brief A class of functions implementing solutions of Travelling Salesman.
 * 
 * Various variants can be implemented here, such as the shortest path past a
 * set of points or of lines.
 * 
 * \tparam E The type of elements that must be ordered by this instance of
 * <em>TravellingSalesman</em>. Note that each element is copied twice in a run
 * of \link findPath, so if this type is difficult to copy, provide pointers to
 * the elements instead.
 */
template<class E> class TravellingSalesman
{
    typedef typename std::list<Waypoint<E>*>::iterator WaypointListIterator; //To help the compiler with templates in templates.

public:
    /*!
     * \brief Constructs an instance of Travelling Salesman.
     * 
     * \param get_orientations A function to get possible orientations for
     * elements in the path. Each orientation defines a possible way that the
     * element could be inserted in the path. To do that it must provide a
     * start point and an end point for each orientation.
     */
    TravellingSalesman(std::function<std::vector<std::pair<Point, Point>>(E)> get_orientations) : get_orientations(get_orientations)
    {
        //Do nothing. All parameters are already copied to fields.
    }

    /*!
     * \brief Destroys the instance, releasing all memory used by it.
     */
    virtual ~TravellingSalesman();

    /*!
     * \brief Computes a short path along all specified elements.
     * 
     * A short path is computed that includes all specified elements, but not
     * always the shortest path. Finding the shortest path is known as the
     * Travelling Salesman Problem, and this is an NP-complete problem. The
     * solution returned by this function is just a heuristic approximation.
     * 
     * The approximation will try to insert random elements at the best location
     * in the current path, thereby incrementally constructing a good path. Each
     * element can be inserted in multiple possible orientations, defined by the
     * <em>get_orientations</em> function.
     * 
     * \param elements The elements past which the path must run.
     * \param element_orientations Output parameter to indicate for each element
     * in which orientation it must be placed to minimise the travel time. The
     * resulting integers correspond to the index of the options given by the
     * <em>get_orientations</em> constructor parameter.
     * \param starting_point A fixed starting point of the path, if any. If this
     * is <em>nullptr</em>, the path may start at the start or end point of any
     * element, depending on which the heuristic deems shortest.
     * \return A vector of elements, in an order that would make a short path.
     */
    std::vector<E> findPath(std::vector<E> elements, std::vector<size_t>& element_orientations, Point* starting_point = nullptr);

protected:
    /*!
     * \brief Function to use to get the possible orientations of an element.
     * 
     * Each orientation has a start point and an end point, in that order.
     */
    std::function<std::vector<std::pair<Point, Point>>(E)> get_orientations;

private:
    /*!
     * \brief Puts all elements in waypoints, caching their endpoints.
     * 
     * The <em>get_start</em> and <em>get_end</em> functions are called on each
     * element. The results are stored along with the element in a waypoint and
     * a pointer to the waypoint is added to the resulting vector.
     * 
     * Note that this creates a waypoint for each element on the heap. These
     * waypoints need to be deleted when the algorithm is done. This is why this
     * function must always stay private.
     * 
     * \param elements The elements to put into waypoints.
     * \return A vector of waypoints with the specified elements in them.
     */
    std::vector<Waypoint<E>*> fillWaypoints(std::vector<E> elements);

    /*!
     * \brief Tries to insert a waypoint at the first position in the list.
     *
     * It will try to insert the waypoint at all possible orientations. If it
     * finds an orientation with an insertion distance that is less than the
     * best distance, it will update the \p best_distance, \p best_orientation
     * and \p best_insert variables with the parameters of this insertion.
     *
     * \param waypoint The waypoint to insert in the list.
     * \param starting_point The starting point to insert the waypoint after, if
     * any.
     * \param first_element The first element of the list to insert the waypoint
     * before.
     * \param best_distance The current best distance of inserting the waypoint.
     * This parameter is changed if a better insertion is found.
     * \param best_orientation The current best orientation of inserting the
     * waypoint. This parameter is changed if a better insertion is found.
     * \param best_insert The current best place to insert the waypoint. This
     * parameter is changed if a better insertion is found.
     */
    inline void tryInsertFirst(Waypoint<E>* waypoint, Point* starting_point, WaypointListIterator first_element, int64_t* best_distance, size_t* best_orientation_index, WaypointListIterator* best_insert);

    /*!
     * \brief Tries to insert a waypoint at the last position in the list.
     *
     * It will try to insert the waypoint at all possible orientations. If it
     * finds an orientation with an insertion distance that is less than the
     * best distance, it will update the \p best_distance, \p best_orientation
     * and \p best_insert variables with the parameters of this insertion.
     *
     * \param waypoint The waypoint to insert in the list.
     * \param last_element The last element of the list to insert the waypoint
     * after.
     * \param The WaypointListIterator to use to indicate that it should insert
     * the new waypoint after the last element of the list, if it should find
     * that as a new best position.
     * \param best_distance The current best distance of inserting the waypoint.
     * This parameter is changed if a better insertion is found.
     * \param best_orientation The current best orientation of inserting the
     * waypoint. This parameter is changed if a better insertion is found.
     * \param best_insert The current best place to insert the waypoint. This
     * parameter is changed if a better insertion is found.
     */
    inline void tryInsertLast(Waypoint<E>* waypoint, WaypointListIterator last_element, WaypointListIterator after_insert, int64_t* best_distance, size_t* best_orientation_index, WaypointListIterator* best_insert);

    /*!
     * \brief Tries to insert a waypoint at a position in the centre of the
     * list.
     *
     * It will try to insert the waypoint at all possible orientations. If it
     * finds an orientation with an insertion distance that is less than the
     * best distance, it will update the \p best_distance, \p best_orientation
     * and \p best_insert variables with the parameters of this insertion.
     *
     * \param waypoint The waypoint to insert in the list.
     * \param before_insert The element after which to insert the new waypoint.
     * \param after_insert The element before which to insert the new waypoint.
     * \param best_distance The current best distance of inserting the waypoint.
     * This parameter is changed if a better insertion is found.
     * \param best_orientation The current best orientation of inserting the
     * waypoint. This parameter is changed if a better insertion is found.
     * \param best_insert The current best place to insert the waypoint. This
     * parameter is changed if a better insertion is found.
     */
    inline void tryInsertMiddle(Waypoint<E>* waypoint, WaypointListIterator before_insert, WaypointListIterator after_insert, int64_t* best_distance, size_t* best_orientation_index, WaypointListIterator* best_insert);
};

////BELOW FOLLOWS THE IMPLEMENTATION.////

template<class E> TravellingSalesman<E>::~TravellingSalesman()
{
    //Do nothing.
}

template<class E> std::vector<E> TravellingSalesman<E>::findPath(std::vector<E> elements, std::vector<size_t>& element_orientations, Point* starting_point)
{
    /* This approximation algorithm of TSP implements the random insertion
     * heuristic. Random insertion has in tests proven to be almost as good as
     * Christofides (111% of the optimal path length rather than 110% on random
     * graphs) but is much faster to compute. */
    if (elements.empty())
    {
        return std::vector<E>();
    }

    auto rng = std::default_random_engine(0xDECAFF); //Always use a fixed seed! Wouldn't want it to be nondeterministic.
    std::vector<Waypoint<E>*> shuffled = fillWaypoints(elements);
    std::shuffle(shuffled.begin(), shuffled.end(), rng); //"Randomly" shuffles the waypoints.

    std::list<Waypoint<E>*> result;

    if (!starting_point) //If there is no starting point, just insert the initial element.
    {
        shuffled[0]->best_orientation_index = 0; //Choose an arbitrary orientation for the first element.
        result.push_back(shuffled[0]); //Due to the check at the start, we know that shuffled always contains at least 1 element.
    }
    else //If there is a starting point, insert the initial element after it.
    {
        int64_t best_distance = std::numeric_limits<int64_t>::max(); //Change in travel distance to insert the waypoint. Minimise this distance by varying the orientation.
        size_t best_orientation_index; //In what orientation to insert the element.
        for (size_t orientation_index = 0; orientation_index < shuffled[0]->orientation_indices.size(); orientation_index++)
        {
            int64_t distance = vSize(*starting_point - shuffled[0]->orientation_indices[orientation_index].first); //Distance from the starting point to the start point of this element.
            if (distance < best_distance)
            {
                best_distance = distance;
                best_orientation_index = orientation_index;
            }
        }
        shuffled[0]->best_orientation_index = best_orientation_index;
        result.push_back(shuffled[0]);
    }

    //Now randomly insert the rest of the points.
    for (size_t next_to_insert = 1; next_to_insert < shuffled.size(); next_to_insert++)
    {
        Waypoint<E>* waypoint = shuffled[next_to_insert];
        int64_t best_distance = std::numeric_limits<int64_t>::max(); //Change in travel distance to insert the waypoint. Minimise this distance by varying the insertion point and orientation.
        WaypointListIterator best_insert; //Where to insert the element. It will be inserted before this element. If it's nullptr, insert at the very front.
        size_t best_orientation_index; //In what orientation to insert the element.

        //First try inserting before the first element.
        tryInsertFirst(waypoint, starting_point, result.begin(), &best_distance, &best_orientation_index, &best_insert);

        //Try inserting at the other positions.
        for (WaypointListIterator before_insert = result.begin(); before_insert != result.end(); before_insert++)
        {
            WaypointListIterator after_insert = before_insert;
            after_insert++; //Get the element after the current element.
            if (after_insert == result.end()) //There is no next element. We're inserting at the end of the path.
            {
                tryInsertLast(waypoint, before_insert, after_insert, &best_distance, &best_orientation_index, &best_insert);
            }
            else //There is a next element. We're inserting somewhere in the middle.
            {
                tryInsertMiddle(waypoint, before_insert, after_insert, &best_distance, &best_orientation_index, &best_insert);
            }
        }

        //Actually insert the waypoint at the best position we found.
        waypoint->best_orientation_index = best_orientation_index;
        if (best_insert == result.end()) //We must insert at the very end.
        {
            result.push_back(waypoint);
        }
        else //We must insert before best_insert.
        {
            result.insert(best_insert, waypoint);
        }
    }

    //Now that we've inserted all points, linearise them into one vector.
    std::vector<E> result_vector;
    result_vector.reserve(elements.size());
    element_orientations.clear(); //Prepare the element_orientations vector for storing in which orientation each element should be placed.
    element_orientations.reserve(elements.size());
    for (Waypoint<E>* waypoint : result)
    {
        result_vector.push_back(waypoint->element);
        element_orientations.push_back(waypoint->best_orientation_index);
        delete waypoint; //Free the waypoint from memory. It is no longer needed from here on, since we copied the element in it to the output.
    }
    return result_vector;
}

template<class E> std::vector<Waypoint<E>*> TravellingSalesman<E>::fillWaypoints(std::vector<E> elements)
{
    std::vector<Waypoint<E>*> result;
    result.reserve(elements.size());

    for (E element : elements) //Put every element in a waypoint.
    {
        Waypoint<E>* waypoint = new Waypoint<E>(get_orientations(element), element); //Yes, this must be deleted when the algorithm is done!
        result.push_back(waypoint);
    }
    return result;
}

template<class E> inline void TravellingSalesman<E>::tryInsertFirst(Waypoint<E>* waypoint, Point* starting_point, WaypointListIterator first_element, int64_t* best_distance, size_t* best_orientation_index, WaypointListIterator* best_insert)
{
    if (!waypoint or !best_distance or !best_orientation_index or !best_insert) //Input checking.
    {
        return;
    }

    for (size_t orientation = 0; orientation < waypoint->orientation_indices.size(); orientation++)
    {
        int64_t before_distance = 0;
        if (starting_point) //If there is a starting point, we're inserting between the first point and the starting point.
        {
            before_distance = vSize(*starting_point - waypoint->orientation_indices[orientation].first);
        }
        const Point end_of_this = waypoint->orientation_indices[orientation].second;
        const Point start_of_first = (*first_element)->orientation_indices[(*first_element)->best_orientation_index].first;
        const int64_t after_distance = vSize(end_of_this - start_of_first); //From the end of this element to the start of the first element.
        const int64_t distance = before_distance + after_distance;
        if (distance < *best_distance)
        {
            *best_distance = distance;
            *best_insert = first_element;
            *best_orientation_index = orientation;
        }
    }
}

template<class E> inline void TravellingSalesman<E>::tryInsertLast(Waypoint<E>* waypoint, WaypointListIterator last_element, WaypointListIterator after_insert, int64_t* best_distance, size_t* best_orientation_index, WaypointListIterator* best_insert)
{
    if (!waypoint or !best_distance or !best_orientation_index or !best_insert) //Input checking.
    {
        return;
    }

    for (size_t orientation = 0; orientation < waypoint->orientation_indices.size(); orientation++)
    {
        const Point end_of_last = (*last_element)->orientation_indices[(*last_element)->best_orientation_index].second;
        const Point start_of_this = waypoint->orientation_indices[orientation].first;
        const int64_t distance = vSize(end_of_last - start_of_this); //From the end of the last element to the start of this element.
        if (distance < *best_distance)
        {
            *best_distance = distance;
            *best_insert = after_insert;
            *best_orientation_index = orientation;
        }
    }
}

template<class E> inline void TravellingSalesman<E>::tryInsertMiddle(Waypoint<E>* waypoint, WaypointListIterator before_insert, WaypointListIterator after_insert, int64_t* best_distance, size_t* best_orientation_index, WaypointListIterator* best_insert)
{
    if (!waypoint or !best_distance or !best_orientation_index or !best_insert) //Input checking.
    {
        return;
    }

    for (size_t orientation = 0; orientation < waypoint->orientation_indices.size(); orientation++)
    {
        const Point end_of_before = (*before_insert)->orientation_indices[(*before_insert)->best_orientation_index].second;
        const Point start_of_after = (*after_insert)->orientation_indices[(*after_insert)->best_orientation_index].first;
        const Point start_of_this = waypoint->orientation_indices[orientation].first;
        const Point end_of_this = waypoint->orientation_indices[orientation].second;
        const int64_t removed_distance = vSize(end_of_before - start_of_after); //Distance of the original move that we'll remove.
        const int64_t before_distance = vSize(end_of_before - start_of_this); //From end of previous element to start of this element.
        const int64_t after_distance = vSize(end_of_this - start_of_after); //From end of this element to start of next element.
        const int64_t distance = before_distance + after_distance - removed_distance;
        if (distance < *best_distance)
        {
            *best_distance = distance;
            *best_insert = after_insert;
            *best_orientation_index = orientation;
        }
    }
}

}

#endif /* TRAVELLINGSALESMAN_H */

