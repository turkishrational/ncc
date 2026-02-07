/* stack.h - simple value stack                    ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#ifndef STACK_H
#define STACK_H

template<class T> class stack
{
    enum
    {
        LOG2_DEFAULT_CAPACITY   = 4,        /* 16 */
        LOG2_MAX_CAPACITY       = 30,       /* 1GB. don't overflow int */

        DEFAULT_CAPACITY        = (1 << LOG2_DEFAULT_CAPACITY),
        MAX_CAPACITY            = (1 << LOG2_MAX_CAPACITY)
    };

    int _sp;            /* index of top */
    int _capacity;      /* ... of _stack[] */

    T *_stack;

public:
    stack()
        : _sp(-1),
          _capacity(DEFAULT_CAPACITY),
          _stack(new T[DEFAULT_CAPACITY]) {}

    ~stack() { delete[] _stack; }

    bool empty() { return (_sp < 0); }
    void pop() { ASSERT(_sp >= 0); --_sp; }
    T& top() { ASSERT(_sp >= 0); return _stack[_sp]; }

    void push(const T& elem);
};


template<class T> void stack<T>::push(const T& elem)
{
    ++_sp;

    if (_sp == _capacity) {
        T *stack;
        int capacity;

        /* technically this is a compiler design limit, but no algorithm
           will ever grow a stack this large unless it's gone haywire. */

        ASSERT(_capacity != MAX_CAPACITY);

        capacity = _capacity << 1;
        stack = new T[capacity];

        /* this is potentially slow- should be implemented as a block copy
           if T is just a pod object. a good optimizer recognize this. it's
           not critical in any case, as growing should be infrequent. */

        for (int i = 0; i < _capacity; ++i)
            stack[i] = _stack[i];

        delete[] _stack;
        _stack = stack;
        _capacity = capacity;
    }

    _stack[_sp] = elem;
}

#endif /* STACK_H */

/* vi: set ts=4 expandtab: */
