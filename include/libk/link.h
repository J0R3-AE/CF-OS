/**
 * @file link.h
 * @brief Intrusive doubly‑linked list implementation for kernel structures.
 *
 * This header provides a minimal, efficient intrusive list system similar
 * to the Linux kernel's list.h. Any structure may embed a `Link` member
 * to participate in one or more doubly‑linked lists without additional
 * allocations.
 *
 * The API supports insertion, removal, movement, and safe iteration.
 */

#pragma once

#include "libk/types.h"
#include <stdbool.h>
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* Core Link Structure                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @struct Link
 * @brief Intrusive doubly‑linked list node.
 *
 * Structures embed this as a member to participate in lists. A `Link`
 * may act as a list head when initialized to point to itself.
 */
typedef struct Link
{
    struct Link *prev; /**< Previous node in the list. */
    struct Link *next; /**< Next node in the list. */
} Link;

/* -------------------------------------------------------------------------- */
/* Basic Operations                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize a link as an empty list head.
 *
 * After initialization, `x->next == x` and `x->prev == x`.
 *
 * @param x Link to initialize.
 */
static inline void LinkInit(Link *x)
{
    x->prev = x;
    x->next = x;
}

/**
 * @brief Insert link @p x immediately after link @p a.
 *
 * @param a Existing link in the list.
 * @param x New link to insert.
 */
static inline void LinkAfter(Link *a, Link *x)
{
    Link *p = a;
    Link *n = a->next;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

/**
 * @brief Insert link @p x immediately before link @p a.
 *
 * @param a Existing link in the list.
 * @param x New link to insert.
 */
static inline void LinkBefore(Link *a, Link *x)
{
    Link *p = a->prev;
    Link *n = a;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

/**
 * @brief Remove link @p x from its list.
 *
 * Does not free memory; simply detaches the node.
 *
 * @param x Link to remove.
 */
static inline void LinkRemove(Link *x)
{
    Link *p = x->prev;
    Link *n = x->next;
    n->prev = p;
    p->next = n;
    x->next = NULL;
    x->prev = NULL;
}

/* -------------------------------------------------------------------------- */
/* Movement Operations                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Move link @p x to be immediately after link @p a.
 *
 * Removes @p x from its current list before inserting.
 *
 * @param a Target position.
 * @param x Link to move.
 */
static inline void LinkMoveAfter(Link *a, Link *x)
{
    LinkRemove(x);
    LinkAfter(a, x);
}

/**
 * @brief Move link @p x to be immediately before link @p a.
 *
 * Removes @p x from its current list before inserting.
 *
 * @param a Target position.
 * @param x Link to move.
 */
static inline void LinkMoveBefore(Link *a, Link *x)
{
    LinkRemove(x);
    LinkBefore(a, x);
}

/**
 * @brief Check whether a list is empty.
 *
 * A list is empty if its head points to itself.
 *
 * @param x List head.
 * @return true if empty, false otherwise.
 */
static inline bool ListIsEmpty(Link *x)
{
    return x->next == x;
}

/* -------------------------------------------------------------------------- */
/* Aliases for readability                                                     */
/* -------------------------------------------------------------------------- */

#define ListInit LinkInit
#define ListAfter LinkAfter
#define ListBefore LinkBefore
#define ListRemove LinkRemove
#define ListMoveAfter LinkMoveAfter
#define ListMoveBefore LinkMoveBefore
#define ListIsEmpty ListIsEmpty

/* -------------------------------------------------------------------------- */
/* Container Access                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieve the parent structure from a Link pointer.
 *
 * Equivalent to Linux's container_of().
 *
 * @param link Pointer to the embedded Link.
 * @param T    Parent structure type.
 * @param m    Member name of the Link inside T.
 */
#define LinkData(link, T, m) \
    (T *)((char *)(link) - offsetof(T, m))

/* -------------------------------------------------------------------------- */
/* Iteration Macros                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Iterate forward over a list of structures containing a Link member.
 *
 * @param it   Iterator variable (structure pointer).
 * @param list List head (Link).
 * @param m    Member name of the Link inside the structure.
 */
#define ListForEach(it, list, m)                     \
    for (it = LinkData((list).next, typeof(*it), m); \
         &it->m != &(list);                          \
         it = LinkData(it->m.next, typeof(*it), m))

/**
 * @brief Safe forward iteration allowing removal of the current element.
 */
#define ListForEachSafe(it, n, list, m)              \
    for (it = LinkData((list).next, typeof(*it), m), \
        n = LinkData(it->m.next, typeof(*it), m);    \
         &it->m != &(list);                          \
         it = n,                                     \
        n = LinkData(n->m.next, typeof(*it), m))

/**
 * @brief Reverse iteration over a list.
 */
#define ListForEachReverse(it, list, m)              \
    for (it = LinkData((list).prev, typeof(*it), m); \
         &it->m != &(list);                          \
         it = LinkData(it->m.prev, typeof(*it), m))

/**
 * @brief Safe reverse iteration allowing removal.
 */
#define ListForEachReverseSafe(it, n, list, m)       \
    for (it = LinkData((list).prev, typeof(*it), m), \
        n = LinkData(it->m.prev, typeof(*it), m);    \
         &it->m != &(list);                          \
         it = n,                                     \
        n = LinkData(n->m.prev, typeof(*it), m))

/**
 * @brief Iterate over entries (alias for forward iteration).
 */
#define ListForEachEntry(it, list, m)                \
    for (it = LinkData((list).next, typeof(*it), m); \
         &it->m != &(list);                          \
         it = LinkData(it->m.next, typeof(*it), m))

/**
 * @brief Safe forward entry iteration allowing removal.
 */
#define ListForEachEntrySafe(it, n, list, m)         \
    for (it = LinkData((list).next, typeof(*it), m), \
        n = LinkData(it->m.next, typeof(*it), m);    \
         &it->m != &(list);                          \
         it = n,                                     \
        n = LinkData(n->m.next, typeof(*it), m))

/**
 * @brief Reverse entry iteration.
 */
#define ListForEachEntryReverse(it, list, m)         \
    for (it = LinkData((list).prev, typeof(*it), m); \
         &it->m != &(list);                          \
         it = LinkData(it->m.prev, typeof(*it), m))

/**
 * @brief Safe reverse entry iteration allowing removal.
 */
#define ListForEachEntryReverseSafe(it, n, list, m)  \
    for (it = LinkData((list).prev, typeof(*it), m), \
        n = LinkData(it->m.prev, typeof(*it), m);    \
         &it->m != &(list);                          \
         it = n,                                     \
        n = LinkData(n->m.prev, typeof(*it), m))
