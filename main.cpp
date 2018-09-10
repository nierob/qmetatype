#include <QtCore>

/******************************************/
/** This is QMetaType for Qt6 prototype. **/
/******************************************/


/* During Qt5 we learned many thing about type information that we need and that we collect.
The current system is quite good, but it has some deficiances.
- The whole registry is keept behind a mutex and it is very central, the mutex usage actually
  shows on profilers
- It is impossible for a user to extend collected data. That is example of QML in which
  a parallel structure has to be created. Similar problem is shared with data stream operators
  registration, and the code that tries to handle that
- The current implementation is quite fragile from BC perspective, we store data that may not
  match the reality after a while.
- Q_DECLARE_METATYPE is confusing for users, it got better as now we strongly recomand to place it
  after the class declaration, but still...
- It is impossible to register types that are not constructible, the orginal requirement comes from
  QVariant and has no sense in QMetaType context
- Distinction between builtins and custom types is confusing and is responsible for growing binary size.
- Unloading plugins is not supported by the QMetaType, we agreed that Qt is not supporting that case
  but people still do it and the current implementation leaks memory. Same happens for dynamically
  created types, there is no way to "unregister" a type, even if such funciton exists.
- More... TODO
*/


int main(int argc, char** argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    return 0;
}