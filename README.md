# hm_poll
Should be a library communicating with Hoymiles microinverters.

This is the first approach to build an independent library.

## Description

Arduino entry points could be found in *hm_comm.cpp*.<br/>
Communication library class is located in *hm_packets.cpp*.<br/>
Other source files are complementary or configuration files.<br/>

The library is intended to be used with various targets.<br/>
Target specific definition and adaption should grow in *hm_target_x.c*.


Contributions of any kind are welcome!
