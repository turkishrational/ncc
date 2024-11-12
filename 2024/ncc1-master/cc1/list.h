/* list.h - intrusive doubly-linked lists          ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#ifndef LIST_H
#define LIST_H

/* a template implementation based on the BSD sys/queue.h LIST macros.

   a list is headed by a single forward pointer. elements are doubly
   linked so an arbitrary element can be removed without the need to
   traverse the list. new elements are added to the list at the head
   of the list. a list may be traversed in the forward direction only. */

template<class T> class list
{
    T *_head;

public:
    struct entry
    {
        friend class list;

    private:
        T *next;
        T **prev;
    };

    list() : _head(0) {}
    bool empty() { return _head == 0; }

    T *first() { return _head; }
    T *next(T *elm, struct entry T::*links) { return (elm->*links).next; }

    void insert(T *elm, struct entry T::*links)
    {
        if ((elm->*links).next = _head)
            (_head->*links).prev = &(elm->*links).next;

        _head = elm;
        (elm->*links).prev = &_head;
    }

    void remove(T *elm, struct entry T::*links)
    {
        if ((elm->*links).next)
            ((elm->*links).next->*links).prev = (elm->*links).prev;

        *(elm->*links).prev = (elm->*links).next;
    }
};

#endif /* LIST_H */

/* vi: set ts=4 expandtab: */
