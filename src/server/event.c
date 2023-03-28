#include "event.h"

void (*eventHttpRespondCallback)(eventHttpRespond *) = NULL;

void eventHttpRespondSetCallback(void (*callback)(eventHttpRespond *)) {
    eventHttpRespondCallback = callback;
}

void eventHttpRespondInvoke(eventHttpRespond *event) {
    if (eventHttpRespondCallback)
        eventHttpRespondCallback(event);
}
