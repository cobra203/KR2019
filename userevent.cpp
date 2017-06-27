#include "userevent.h"

QEvent::Type UserEvent::e_Type = QEvent::None;

UserEvent::UserEvent() : QEvent(GetType())
{
    e_UserType = USER_EVENT_NOTHING;
}

UserEvent::~UserEvent()
{

}

QEvent::Type UserEvent::GetType()
{
    if(QEvent::None == e_Type) {
        e_Type = (QEvent::Type)QEvent::registerEventType();
    }

    return e_Type;
}
