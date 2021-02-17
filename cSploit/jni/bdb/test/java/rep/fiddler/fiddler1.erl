%%% Some very simple glue for starting fiddler from the command line.

-module(fiddler1).
-export([start/0, start3/0, startn/1]).

start() ->
    fiddler:start(8000,[{7001,6001},{7000,6000}]).

start3() ->
    fiddler:start([{7001,6001},{7000,6000},{7002,6002}]).

%%%
%%% Allows all TCP ports to be configured on the command line.  The
%%% syntax would be something like:
%%%
%%%    erl -noshell -s fiddler1 startn 8000 '[{7001,6001},{7000,6000}]'
%%%
%%% where "8000" is the manager port, and the list of tuples gives the
%%% pass-through configuration.
%%%
startn([MP|[Cfg|[]]]) ->
    MgrPort = list_to_integer(atom_to_list(MP)),
    {ok,Tokens,_} = erl_scan:string(lists:append(atom_to_list(Cfg), ".")),
    {ok,Config} = erl_parse:parse_term(Tokens),
    fiddler:start(MgrPort, Config).
