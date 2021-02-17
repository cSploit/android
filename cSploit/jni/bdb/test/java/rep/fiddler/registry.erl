%%% The Registry is a gen_server process that keeps track of the
%%% currently active set of connection-path processes.  The listeners
%%% add the processes to the list as they're created.  The manager
%%% looks stuff up in order to send commands to the proper place.
%%%
%%% Note that "Count" is a count of the number of paths we've *ever*
%%% registered, not the current length of the list.  (Otherwise I
%%% would have simply called lists:length/1 when needed.)  It's used
%%% to tag each entry with an identifying serial number, I guess.

-module(registry).
-behaviour(gen_server).
-export([start/0,lookup/1,register/1,unregister/1,all/0]).
-export([init/1, handle_call/3]).
-export([code_change/3,handle_cast/2,handle_info/2,terminate/2]).
-import(lists,[dropwhile/2]).

start() ->
    {ok,_Pid} = gen_server:start({local,registry}, registry, [], []),
    ok.

init(_) ->
    {ok,{[],0}}.

handle_call({register,Path, Pid}, _From, State) ->
    {Paths,Count} = State,
    Num = Count + 1,
    {reply, ok, {[{Num,Path,Pid}|Paths],Count+1}};

%% Someday we may want to look up by Id, but for now no one is using it.
handle_call({lookup,Query}, _, State) ->
    {Paths,_Count} = State,
    Ans = [Pid || {_Num, Path, Pid} <- Paths, util:match(Path, Query)],
    {reply, Ans, State};

handle_call({unregister,Path}, _, State) ->
    {Paths,Count} = State,
    {reply, ok, {lists:keydelete(Path, 2, Paths),Count}};

handle_call(all, _, State) ->
    {Paths,_Count} = State,
    {reply, Paths, State}.

lookup(Query) ->
    gen_server:call(registry, {lookup, Query}).

register(Path) ->
    gen_server:call(registry, {register, Path, self()}).

unregister(Path) ->
    gen_server:call(registry, {unregister, Path}).

all() ->
    gen_server:call(registry, all).

%%%
%%% The following functions defined in the gen_server behavior are not
%%% used.  But we include them in order to avoid distracting
%%% compilation warnings.
%%% 
code_change(_,_,_) ->
    ok.

handle_cast(_,_) ->
    ok.

handle_info(_,_) ->
    ok.

terminate(_,_) ->
    ok.
