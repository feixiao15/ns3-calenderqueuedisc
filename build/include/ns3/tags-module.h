#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_TAGS
    // Module headers: 
    #include <ns3/tags.h>
    #include <ns3/tags-helper.h>
#endif 