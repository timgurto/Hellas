// (C) 2016 Tim Gurto

#ifndef ACCESSOR_H
#define ACCESSOR_H

/* Provides full access to an object, to assist with testing.
   The target class will need to declare Accessor as a friend.
   e.g.,
    class Widget{
        int _x;
        template<typename T> friend class Accessor;
    };
    Widget w;
    Accessor<Widget> a(w);
    a->_x = 0;
*/
template<typename T>
class Accessor{
    T &_obj;

public:
    Accessor(T &arg): _obj(arg) {}

    T *operator->() { return &_obj; }
};

#endif
