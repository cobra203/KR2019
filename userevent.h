#ifndef USEREVENT_H
#define USEREVENT_H

#include <QEvent>
#include <qevent.h>

class UserEvent : public QEvent
{
public:
    UserEvent();
    ~UserEvent();

    static QEvent::Type e_Type;

    enum USER_EVENT_TYPE_E {
        USER_EVENT_NOTHING,
        USER_EVENT_ASSERT_ERR,
    } e_UserType;

    static QEvent::Type GetType();
};

#endif // USEREVENT_H
