%% repmgr envelope types
%% 
-define(ACK, 1).
-define(HANDSHAKE, 2).
-define(REP_MESSAGE, 3).
-define(HEARTBEAT, 4).
-define(APP_MSG, 5).
-define(APP_RESPONSE, 6).
-define(RESP_ERROR, 7).
-define(OWN_MSG, 8).

%% Replication (control) message types
%% 
-define(PAGE, 19).

-define(REPVERSION_47, 5).
-define(REPMGR_VERSION, 4).                     % repmgr's (current) protocol version
