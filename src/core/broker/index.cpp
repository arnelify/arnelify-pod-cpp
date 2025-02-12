#ifndef ARNELIFY_BROKER_IMPORT_CPP
#define ARNELIFY_BROKER_IMPORT_CPP

#ifdef WATCH_SERVER_CPP
#include "src/index.cpp"
#else
#include "src/cpp/index.cpp"
#endif

ArnelifyBroker* broker = new ArnelifyBroker();

#endif