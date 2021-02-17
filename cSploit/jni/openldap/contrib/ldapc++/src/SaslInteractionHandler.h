// $OpenLDAP$
/*
 * Copyright 2007-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef SASL_INTERACTION_HANDLER_H
#define SASL_INTERACTION_HANDLER_H
#include <list>

class SaslInteraction;

class SaslInteractionHandler {
    public:
        virtual void handleInteractions( const std::list<SaslInteraction*> &cb )=0;
        virtual ~SaslInteractionHandler() {}
};

class DefaultSaslInteractionHandler {
    public:
        virtual void handleInteractions( const std::list<SaslInteraction*> &cb );
        virtual ~DefaultSaslInteractionHandler();

    private:
        std::list<SaslInteraction*> cleanupList;
};
#endif /* SASL_INTERACTION_HANDLER_H */
